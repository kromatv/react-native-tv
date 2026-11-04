#include "Convert.h"

#include "Invoke.h"
#include "RuntimeState.h"
#include "SharedObjectRegistry.h"

#include <SharedObject.h>

#include <cmath>
#include <cstring>

namespace rntizen::expo::detail {

using facebook::jsi::JSError;
using facebook::jsi::Runtime;
using facebook::jsi::Value;

namespace {

bool isByteCount(double value) {
  return std::isfinite(value) && value >= 0;
}

} // namespace

JSError typeError(Runtime &runtime, const char *expected) {
  return JSError(runtime, std::string("Expected ") + expected);
}

Value bytesToJs(Runtime &runtime, const Bytes &bytes) {
  auto array = runtime.global()
                   .getPropertyAsFunction(runtime, "Uint8Array")
                   .callAsConstructor(runtime, static_cast<double>(bytes.data.size()))
                   .getObject(runtime);
  if (!bytes.data.empty()) {
    auto buffer = array.getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
    std::memcpy(buffer.data(runtime), bytes.data.data(), bytes.data.size());
  }
  return Value(runtime, array);
}

std::optional<Bytes> bytesFromJs(Runtime &runtime, const Value &value) {
  if (!value.isObject()) return std::nullopt;
  auto object = value.getObject(runtime);
  if (object.isArrayBuffer(runtime)) {
    auto buffer = object.getArrayBuffer(runtime);
    return Bytes{std::string(reinterpret_cast<const char *>(buffer.data(runtime)), buffer.size(runtime))};
  }
  auto buffer = object.getProperty(runtime, "buffer");
  auto offset = object.getProperty(runtime, "byteOffset");
  auto length = object.getProperty(runtime, "byteLength");
  if (!buffer.isObject() || !offset.isNumber() || !length.isNumber()) return std::nullopt;
  auto arrayBuffer = buffer.getObject(runtime);
  if (!arrayBuffer.isArrayBuffer(runtime)) return std::nullopt;
  auto bytes = arrayBuffer.getArrayBuffer(runtime);
  const auto start = offset.getNumber();
  const auto count = length.getNumber();
  if (!isByteCount(start) || !isByteCount(count) || start + count > static_cast<double>(bytes.size(runtime))) return std::nullopt;
  const auto *data = reinterpret_cast<const char *>(bytes.data(runtime));
  return Bytes{std::string(data + static_cast<std::size_t>(start), static_cast<std::size_t>(count))};
}

Value sharedObjectToJs(Runtime &runtime, const std::shared_ptr<SharedObject> &object) {
  return RuntimeState::get(runtime).objectFor(runtime, object);
}

std::shared_ptr<SharedObject> sharedObjectFromJs(Runtime &runtime, const Value &value) {
  auto &registry = SharedObjectRegistry::shared();
  if (value.isNumber()) return registry.find(static_cast<SharedObjectId>(value.getNumber()));
  if (!value.isObject()) return nullptr;
  auto object = value.getObject(runtime);
  if (!object.hasNativeState<::expo::SharedObject::NativeState>(runtime)) return nullptr;
  return registry.find(object.getNativeState<::expo::SharedObject::NativeState>(runtime)->objectId);
}

const Value &argumentAt(std::span<const Value> arguments, size_t index) {
  static const Value undefined;
  return index < arguments.size() ? arguments[index] : undefined;
}

Value errorFor(Runtime &runtime, const std::exception &error) {
  const auto *coded = dynamic_cast<const CodedError *>(&error);
  return makeError(runtime, error.what(), coded != nullptr ? coded->code() : std::string());
}

PromiseOutcome failureFor(const std::exception &error) {
  const auto *coded = dynamic_cast<const CodedError *>(&error);
  return {.value = nullptr, .error = std::string(error.what()), .code = coded != nullptr ? coded->code() : std::string()};
}

} // namespace rntizen::expo::detail
