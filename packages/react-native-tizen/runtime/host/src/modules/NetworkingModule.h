#pragma once

#include "../network/HttpEngine.h"
#include "ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>

namespace rntizen {

/// `Networking`, under React Native's XMLHttpRequest and therefore `fetch`.
/// Requests run on the HttpEngine and answer with the device events
/// XMLHttpRequest listens for. Bodies are text, base64, a blob, form data or a
/// `file:` or `data:` URI, and a `blob:` URL is answered from the BlobStore.
class NetworkingModule final : public facebook::react::NativeNetworkingAndroidCxxSpec<NetworkingModule> {
 public:
  NetworkingModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  void sendRequest(
      facebook::jsi::Runtime &runtime,
      facebook::jsi::String method,
      facebook::jsi::String url,
      double requestId,
      facebook::jsi::Array headers,
      facebook::jsi::Object data,
      facebook::jsi::String responseType,
      bool useIncrementalUpdates,
      double timeout,
      bool withCredentials);
  void abortRequest(facebook::jsi::Runtime &runtime, double requestId);
  void clearCookies(facebook::jsi::Runtime &runtime, facebook::jsi::Function callback);
  void addListener(facebook::jsi::Runtime &runtime, facebook::jsi::String eventName);
  void removeListeners(facebook::jsi::Runtime &runtime, double count);

 private:
  HttpResponseHandler responseHandler(int requestId, const std::string &responseType, bool incremental) const;

  std::function<void(const std::string &, folly::dynamic)> emitDeviceEvent_;
  HttpEngine engine_;
};

} // namespace rntizen
