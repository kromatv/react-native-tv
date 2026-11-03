#include "NetworkingModule.h"

#include "PlatformModuleRegistry.h"
#include "blob/BlobReference.h"
#include "blob/BlobStore.h"

#include "../network/Base64.h"

#include <jsi/JSIDynamic.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <system_error>

namespace rntizen {

namespace {

using facebook::jsi::Array;
using facebook::jsi::Runtime;

std::vector<std::pair<std::string, std::string>> headerPairs(Runtime &runtime, const Array &headers) {
  std::vector<std::pair<std::string, std::string>> pairs;
  for (std::size_t index = 0; index < headers.size(runtime); ++index) {
    auto pair = headers.getValueAtIndex(runtime, index).asObject(runtime).asArray(runtime);
    pairs.emplace_back(
        pair.getValueAtIndex(runtime, 0).toString(runtime).utf8(runtime),
        pair.getValueAtIndex(runtime, 1).toString(runtime).utf8(runtime));
  }
  return pairs;
}

constexpr std::string_view kBlobScheme = "blob:";

std::optional<std::string> requestBody(Runtime &runtime, const facebook::jsi::Object &data) {
  if (auto text = data.getProperty(runtime, "string"); text.isString()) return text.getString(runtime).utf8(runtime);
  if (auto encoded = data.getProperty(runtime, "base64"); encoded.isString()) {
    return decodeBase64(encoded.getString(runtime).utf8(runtime));
  }
  if (auto blob = data.getProperty(runtime, "blob"); blob.isObject()) {
    return resolveBlob(readBlobReference(runtime, blob.getObject(runtime)));
  }
  return std::nullopt;
}

std::optional<std::size_t> parsedSize(std::string_view digits) {
  std::size_t value = 0;
  if (std::from_chars(digits.data(), digits.data() + digits.size(), value).ec != std::errc()) return std::nullopt;
  return value;
}

std::optional<std::size_t> queryNumber(std::string_view query, std::string_view name) {
  std::size_t start = 0;
  while (start < query.size()) {
    const auto end = std::min(query.find('&', start), query.size());
    if (const auto pair = query.substr(start, end - start);
        pair.starts_with(name) && pair.size() > name.size() && pair[name.size()] == '=') {
      return parsedSize(pair.substr(name.size() + 1));
    }
    start = end + 1;
  }
  return std::nullopt;
}

std::optional<std::string> blobUrlBytes(std::string_view url) {
  auto rest = url.substr(kBlobScheme.size());
  if (rest.starts_with("//")) rest = rest.substr(std::min(rest.find('/', 2), rest.size() - 1) + 1);
  const auto question = rest.find('?');
  const auto query = question == std::string_view::npos ? std::string_view() : rest.substr(question + 1);
  return BlobStore::shared().resolve(
      std::string(rest.substr(0, question)), queryNumber(query, "offset").value_or(0), queryNumber(query, "size"));
}

std::string contentType(const folly::dynamic &headers) {
  for (const auto &[name, value] : headers.items()) {
    auto key = name.asString();
    std::ranges::transform(key, key.begin(), [](unsigned char c) { return std::tolower(c); });
    if (key == "content-type") return value.asString();
  }
  return {};
}

void answerFromBlob(const std::string &url, const HttpResponseHandler &handler) {
  auto bytes = blobUrlBytes(url);
  if (!bytes) {
    handler.onComplete("The blob " + url + " does not exist", false);
    return;
  }
  handler.onResponse(200, folly::dynamic::object(), url);
  handler.onBody(std::move(*bytes));
  handler.onComplete({}, false);
}

} // namespace

NetworkingModule::NetworkingModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context)
    : NativeNetworkingAndroidCxxSpec(std::move(jsInvoker)), emitDeviceEvent_(context.emitDeviceEvent) {}

HttpResponseHandler NetworkingModule::responseHandler(
    int requestId,
    const std::string &responseType,
    bool incremental) const {
  auto emit = emitDeviceEvent_;
  auto type = std::make_shared<std::string>();
  return HttpResponseHandler{
      .onResponse =
          [emit, requestId, type](long status, folly::dynamic headers, std::string url) {
            *type = contentType(headers);
            emit("didReceiveNetworkResponse", folly::dynamic::array(requestId, status, std::move(headers), url));
          },
      .onBody =
          [emit, requestId, responseType, incremental, type](std::string body) {
            if (responseType == "blob") {
              const auto size = static_cast<int64_t>(body.size());
              auto blob = folly::dynamic::object("blobId", BlobStore::shared().store(std::move(body)))("offset", 0)(
                  "size", size)("type", *type);
              emit("didReceiveNetworkData", folly::dynamic::array(requestId, std::move(blob)));
              return;
            }
            auto payload = responseType == "base64" ? encodeBase64(body) : std::move(body);
            if (incremental) {
              const auto size = static_cast<int64_t>(payload.size());
              emit("didReceiveNetworkIncrementalData",
                   folly::dynamic::array(requestId, std::move(payload), size, size));
            } else {
              emit("didReceiveNetworkData", folly::dynamic::array(requestId, std::move(payload)));
            }
          },
      .onComplete =
          [emit, requestId](std::string error, bool timedOut) {
            emit("didCompleteNetworkResponse", folly::dynamic::array(requestId, std::move(error), timedOut));
          },
  };
}

void NetworkingModule::sendRequest(
    Runtime &runtime,
    facebook::jsi::String method,
    facebook::jsi::String url,
    double requestId,
    Array headers,
    facebook::jsi::Object data,
    facebook::jsi::String responseType,
    bool useIncrementalUpdates,
    double timeout,
    bool) {
  const auto id = static_cast<int>(requestId);
  const auto type = responseType.utf8(runtime);
  auto target = url.utf8(runtime);
  auto handler = responseHandler(id, type, useIncrementalUpdates && type == "text");
  if (target.rfind(kBlobScheme, 0) == 0) {
    answerFromBlob(target, handler);
    return;
  }
  engine_.start(
      HttpRequest{
          .id = id,
          .method = method.utf8(runtime),
          .url = std::move(target),
          .headers = headerPairs(runtime, headers),
          .body = requestBody(runtime, data),
          .timeout = std::chrono::milliseconds(static_cast<int64_t>(timeout)),
      },
      std::move(handler));
}

void NetworkingModule::abortRequest(Runtime &, double requestId) {
  engine_.cancel(static_cast<int>(requestId));
}

void NetworkingModule::clearCookies(Runtime &, facebook::jsi::Function callback) {
  auto invoker = jsInvoker_;
  auto shared = std::make_shared<facebook::jsi::Function>(std::move(callback));
  engine_.clearCookies([invoker, shared] {
    invoker->invokeAsync([shared](Runtime &runtime) { shared->call(runtime, true); });
  });
}

void NetworkingModule::addListener(Runtime &, facebook::jsi::String) {}

void NetworkingModule::removeListeners(Runtime &, double) {}

RN_TIZEN_PLATFORM_MODULE("Networking", NetworkingModule);

} // namespace rntizen
