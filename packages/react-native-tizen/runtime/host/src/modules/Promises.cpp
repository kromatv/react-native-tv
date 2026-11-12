#include "Promises.h"

namespace rntizen {

namespace {

facebook::jsi::Value callOnPromise(
    facebook::jsi::Runtime &runtime,
    const char *method,
    const facebook::jsi::Value &argument) {
  auto promise = runtime.global().getPropertyAsObject(runtime, "Promise");
  return promise.getPropertyAsFunction(runtime, method).callWithThis(runtime, promise, argument);
}

} // namespace

facebook::jsi::Value resolvedPromise(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &value) {
  return callOnPromise(runtime, "resolve", value);
}

facebook::jsi::Value rejectedPromise(facebook::jsi::Runtime &runtime, const std::string &message) {
  auto error = runtime.global().getPropertyAsFunction(runtime, "Error").callAsConstructor(
      runtime, facebook::jsi::String::createFromUtf8(runtime, message));
  return callOnPromise(runtime, "reject", error);
}

facebook::jsi::Value rejectedPromiseWith(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &reason) {
  return callOnPromise(runtime, "reject", reason);
}

} // namespace rntizen
