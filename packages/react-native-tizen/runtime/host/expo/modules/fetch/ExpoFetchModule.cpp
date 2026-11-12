#include "NativeResponse.h"

#include "../../ExpoModuleRegistry.h"

#include "modules/blob/BlobStore.h"
#include "network/HttpEngine.h"

#include <folly/Indestructible.h>

#include <atomic>

namespace rntizen::expo {

namespace {

HttpEngine &engine() {
  static folly::Indestructible<HttpEngine> shared;
  return *shared;
}

std::string stringField(const folly::dynamic &object, const char *name, const char *fallback) {
  const auto *field = object.isObject() ? object.get_ptr(name) : nullptr;
  return field != nullptr && field->isString() ? field->getString() : fallback;
}

std::vector<std::pair<std::string, std::string>> requestHeaders(const folly::dynamic &init) {
  std::vector<std::pair<std::string, std::string>> headers;
  const auto *list = init.isObject() ? init.get_ptr("headers") : nullptr;
  if (list == nullptr || !list->isArray()) return headers;
  for (const auto &pair : *list) {
    if (pair.isArray() && pair.size() == 2 && pair[0].isString() && pair[1].isString()) {
      headers.emplace_back(pair[0].getString(), pair[1].getString());
    }
  }
  return headers;
}

std::vector<std::pair<std::string, std::string>> responseHeaders(const folly::dynamic &headers) {
  std::vector<std::pair<std::string, std::string>> pairs;
  if (!headers.isObject()) return pairs;
  for (const auto &[name, value] : headers.items()) {
    if (name.isString() && value.isString()) pairs.emplace_back(name.getString(), value.getString());
  }
  return pairs;
}

class NativeRequest final : public SharedObject {
 public:
  explicit NativeRequest(std::shared_ptr<NativeResponse> response) : response_(std::move(response)) {}

  void start(const std::string &url, const folly::dynamic &init, std::optional<Bytes> body, Promise promise) {
    static std::atomic nextId{1};
    if (!response_->awaitHead(std::move(promise))) return;
    const int id = nextId++;
    id_ = id;
    auto response = response_;
    engine().start(
        HttpRequest{
            .id = id,
            .method = stringField(init, "method", "GET"),
            .url = url,
            .headers = requestHeaders(init),
            .body = body ? std::optional<std::string>(std::move(body->data)) : std::nullopt,
            .timeout = std::chrono::milliseconds(0),
        },
        HttpResponseHandler{
            .onResponse = [response, url](long status, const folly::dynamic &headers, std::string finalUrl) {
              const bool redirected = finalUrl != url;
              response->receiveHead(status, responseHeaders(headers), std::move(finalUrl), redirected);
            },
            .onBody = [response](const std::string &data) { response->receiveBody(data); },
            .onComplete = [response](const std::string &error, bool) { response->complete(error); },
        });
  }

  void cancel() const {
    if (const int id = id_; id != 0) engine().cancel(id);
    response_->complete("The request was canceled");
  }

 private:
  const std::shared_ptr<NativeResponse> response_;
  std::atomic<int> id_{0};
};

class ExpoFetchModule final : public Module {
 public:
  ModuleDefinition definition() override {
    auto shared = context().shared_from_this();
    return ModuleDefinition("ExpoFetchModule")
        .sharedClass(nativeResponseClass(shared))
        .sharedClass(ClassDefinition<NativeRequest>("NativeRequest")
                         .constructor([](std::shared_ptr<NativeResponse> response) {
                           return std::make_shared<NativeRequest>(std::move(response));
                         })
                         .asyncFunction(
                             "start",
                             [](NativeRequest &request,
                                const std::string &url,
                                const folly::dynamic &init,
                                std::optional<Bytes> body,
                                Promise promise) { request.start(url, init, std::move(body), std::move(promise)); })
                         .function("cancel", [](const NativeRequest &request) { request.cancel(); }))
        .asyncFunction(
            "unstable_createBlobData", [](Bytes bytes) { return BlobStore::shared().store(std::move(bytes.data)); });
  }
};

} // namespace

RN_TIZEN_EXPO_MODULE(ExpoFetchModule, ExpoFetchModule);

} // namespace rntizen::expo
