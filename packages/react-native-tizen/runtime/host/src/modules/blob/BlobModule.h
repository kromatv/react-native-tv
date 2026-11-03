#pragma once

#include "../ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>
#include <ReactCommon/TurboModuleWithJSIBindings.h>

#include <string>

namespace rntizen {

/// `BlobModule`, behind React Native's `Blob`, `URL.createObjectURL` and blob
/// responses. The bytes live in the BlobStore; `Networking` answers blob
/// requests and `blob:` URLs from it, and a WebSocket delivers binary messages
/// as blobs once asked. Like Android's module it installs
/// `global.__blobCollectorProvider`, so a Blob the garbage collector frees
/// releases its bytes.
class BlobModule final : public facebook::react::NativeBlobModuleCxxSpec<BlobModule>,
                         public facebook::react::TurboModuleWithJSIBindings {
 public:
  BlobModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  facebook::jsi::Object getConstants(facebook::jsi::Runtime &runtime);
  void addNetworkingHandler(facebook::jsi::Runtime &runtime);
  void addWebSocketHandler(facebook::jsi::Runtime &runtime, double id);
  void removeWebSocketHandler(facebook::jsi::Runtime &runtime, double id);
  void sendOverSocket(facebook::jsi::Runtime &runtime, facebook::jsi::Object blob, double socketID);
  void createFromParts(facebook::jsi::Runtime &runtime, facebook::jsi::Array parts, std::string withId);
  void release(facebook::jsi::Runtime &runtime, std::string blobId);

 private:
  void installJSIBindingsWithRuntime(facebook::jsi::Runtime &runtime) override;
};

} // namespace rntizen
