#include "NativeResponse.h"

namespace rntizen::expo {

NativeResponse::NativeResponse(std::shared_ptr<Context> context) : context_(std::move(context)) {}

bool NativeResponse::awaitHead(Promise promise) {
  std::string error;
  {
    std::lock_guard lock(mutex_);
    if (!completed_) {
      head_ = std::move(promise);
      return true;
    }
    error = error_.value_or("The request ended without a response");
  }
  promise.reject("ERR_FETCH_FAILED", error);
  return false;
}

void NativeResponse::receiveHead(
    long status,
    std::vector<std::pair<std::string, std::string>> headers,
    std::string url,
    bool redirected) {
  std::optional<Promise> head;
  {
    std::lock_guard lock(mutex_);
    status_ = status;
    headers_ = std::move(headers);
    url_ = std::move(url);
    redirected_ = redirected;
    head = std::exchange(head_, std::nullopt);
  }
  if (head) head->resolve();
}

void NativeResponse::receiveBody(std::string_view body) {
  std::lock_guard lock(mutex_);
  body_ += body;
}

void NativeResponse::complete(const std::string &error) {
  std::optional<Promise> head;
  std::vector<Reader> readers;
  bool streaming = false;
  std::string body;
  {
    std::lock_guard lock(mutex_);
    if (completed_) return;
    completed_ = true;
    if (!error.empty()) error_ = error;
    head = std::exchange(head_, std::nullopt);
    readers = std::exchange(readers_, {});
    streaming = streaming_;
    if (streaming) body = body_;
  }
  if (head) {
    head->reject("ERR_FETCH_FAILED", error.empty() ? "The request ended without a response" : error);
    return;
  }
  for (const auto &reader : readers) settle(reader);
  if (streaming) {
    if (error.empty()) {
      emit("didReceiveResponseData", [bytes = Bytes{body}](auto &runtime) { return Convert<Bytes>::toJs(runtime, bytes); });
      emit("didComplete", nullptr);
    } else {
      emit("didFailWithError", [error](auto &runtime) { return Convert<std::string>::toJs(runtime, error); });
    }
  }
  emit("readyForJSFinalization", nullptr);
}

folly::dynamic NativeResponse::rawHeaders() const {
  std::lock_guard lock(mutex_);
  folly::dynamic headers = folly::dynamic::array();
  for (const auto &[name, value] : headers_) headers.push_back(folly::dynamic::array(name, value));
  return headers;
}

long NativeResponse::status() const {
  std::lock_guard lock(mutex_);
  return status_;
}

std::string NativeResponse::url() const {
  std::lock_guard lock(mutex_);
  return url_;
}

bool NativeResponse::redirected() const {
  std::lock_guard lock(mutex_);
  return redirected_;
}

bool NativeResponse::bodyUsed() const {
  std::lock_guard lock(mutex_);
  return bodyUsed_;
}

void NativeResponse::startStreaming(const Promise &promise) {
  std::optional<Bytes> body;
  {
    std::lock_guard lock(mutex_);
    bodyUsed_ = true;
    if (completed_ && !error_) {
      body = Bytes{body_};
    } else {
      streaming_ = true;
    }
  }
  promise.resolve(std::move(body));
}

void NativeResponse::stopStreaming() {
  std::lock_guard lock(mutex_);
  streaming_ = false;
}

void NativeResponse::readBody(Promise promise, bool asText) {
  Reader reader{.promise = std::move(promise), .asText = asText};
  {
    std::lock_guard lock(mutex_);
    bodyUsed_ = true;
    if (!completed_) {
      readers_.push_back(std::move(reader));
      return;
    }
  }
  settle(reader);
}

void NativeResponse::settle(const Reader &reader) const {
  std::string body;
  {
    std::lock_guard lock(mutex_);
    if (error_) {
      reader.promise.reject("ERR_FETCH_FAILED", *error_);
      return;
    }
    body = body_;
  }
  if (reader.asText) {
    reader.promise.resolve(std::move(body));
  } else {
    reader.promise.resolve(ArrayBufferBytes{std::move(body)});
  }
}

void NativeResponse::emit(const std::string &event, JsValueFactory payload) const {
  context_->emit(std::const_pointer_cast<SharedObject>(shared_from_this()), event, std::move(payload));
}

ClassDefinition<NativeResponse> nativeResponseClass(std::shared_ptr<Context> context) {
  using Response = NativeResponse;
  return ClassDefinition<Response>("NativeResponse")
      .constructor([context] { return std::make_shared<Response>(context); })
      .property("bodyUsed", [](const Response &response) { return response.bodyUsed(); })
      .property("_rawHeaders", [](const Response &response) { return response.rawHeaders(); })
      .property("status", [](const Response &response) { return response.status(); })
      .property("statusText", [](const Response &) { return std::string(); })
      .property("url", [](const Response &response) { return response.url(); })
      .property("redirected", [](const Response &response) { return response.redirected(); })
      .asyncFunction(
          "startStreaming", [](Response &response, const Promise &promise) { response.startStreaming(promise); })
      .function("cancelStreaming", [](Response &response, const std::string &) { response.stopStreaming(); })
      .asyncFunction("arrayBuffer", [](Response &response, const Promise &promise) { response.readBody(promise, false); })
      .asyncFunction("text", [](Response &response, const Promise &promise) { response.readBody(promise, true); });
}

} // namespace rntizen::expo
