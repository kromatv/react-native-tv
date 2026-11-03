#include "BlobModule.h"

#include "../PlatformModuleRegistry.h"
#include "../websocket/WebSocketSockets.h"
#include "BlobReference.h"
#include "BlobStore.h"

namespace rntizen {

namespace {

using facebook::jsi::Runtime;

constexpr const char *kBlobUriScheme = "blob";

class BlobCollector final : public facebook::jsi::HostObject {
 public:
  explicit BlobCollector(std::string blobId) : blobId_(std::move(blobId)) {}
  BlobCollector(const BlobCollector &) = delete;
  BlobCollector &operator=(const BlobCollector &) = delete;
  ~BlobCollector() override {
    BlobStore::shared().remove(blobId_);
  }

 private:
  const std::string blobId_;
};

std::string partBytes(Runtime &runtime, const facebook::jsi::Object &part) {
  const auto type = part.getProperty(runtime, "type").asString(runtime).utf8(runtime);
  if (type == "string") return part.getProperty(runtime, "data").asString(runtime).utf8(runtime);
  if (type == "blob") {
    auto bytes = resolveBlob(readBlobReference(runtime, part.getPropertyAsObject(runtime, "data")));
    if (!bytes) throw facebook::jsi::JSError(runtime, "The specified blob is invalid");
    return std::move(*bytes);
  }
  throw facebook::jsi::JSError(runtime, "Invalid type for blob: " + type);
}

} // namespace

BlobModule::BlobModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &)
    : NativeBlobModuleCxxSpec(std::move(jsInvoker)) {}

facebook::jsi::Object BlobModule::getConstants(Runtime &runtime) {
  auto constants = facebook::jsi::Object(runtime);
  constants.setProperty(runtime, "BLOB_URI_SCHEME", kBlobUriScheme);
  constants.setProperty(runtime, "BLOB_URI_HOST", facebook::jsi::Value::null());
  return constants;
}

void BlobModule::addNetworkingHandler(Runtime &) {}

void BlobModule::addWebSocketHandler(Runtime &, double id) {
  WebSocketSockets::shared().setDeliversBlobs(static_cast<int>(id), true);
}

void BlobModule::removeWebSocketHandler(Runtime &, double id) {
  WebSocketSockets::shared().setDeliversBlobs(static_cast<int>(id), false);
}

void BlobModule::sendOverSocket(Runtime &runtime, facebook::jsi::Object blob, double socketID) {
  auto connection = WebSocketSockets::shared().find(static_cast<int>(socketID));
  if (!connection) return;
  if (auto bytes = resolveBlob(readBlobReference(runtime, blob))) {
    connection->send(WebSocketOpcode::Binary, std::move(*bytes));
  }
}

void BlobModule::createFromParts(Runtime &runtime, facebook::jsi::Array parts, const std::string &withId) {
  std::string bytes;
  for (std::size_t index = 0; index < parts.size(runtime); ++index) {
    bytes += partBytes(runtime, parts.getValueAtIndex(runtime, index).asObject(runtime));
  }
  BlobStore::shared().store(withId, std::move(bytes));
}

void BlobModule::release(Runtime &, const std::string &blobId) {
  BlobStore::shared().remove(blobId);
}

void BlobModule::installJSIBindingsWithRuntime(Runtime &runtime) {
  runtime.global().setProperty(
      runtime,
      "__blobCollectorProvider",
      facebook::jsi::Function::createFromHostFunction(
          runtime,
          facebook::jsi::PropNameID::forAscii(runtime, "__blobCollectorProvider"),
          1,
          [](Runtime &runtime, const facebook::jsi::Value &, const facebook::jsi::Value *arguments, std::size_t count) {
            if (count < 1 || !arguments[0].isString()) return facebook::jsi::Value::null();
            auto blobId = arguments[0].getString(runtime).utf8(runtime);
            const auto size = BlobStore::shared().size(blobId);
            auto collector =
                facebook::jsi::Object::createFromHostObject(runtime, std::make_shared<BlobCollector>(blobId));
            collector.setExternalMemoryPressure(runtime, size);
            return facebook::jsi::Value(runtime, collector);
          }));
}

RN_TIZEN_PLATFORM_MODULE("BlobModule", BlobModule);

} // namespace rntizen
