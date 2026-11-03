#pragma once

#include "../ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>

#include <optional>
#include <string>

namespace rntizen {

/// `WebSocketModule`, under React Native's `WebSocket`. Each socket is a
/// WebSocketConnection on its own thread, reporting the device events
/// `WebSocket.js` listens for: `websocketOpen`, `websocketMessage` (text,
/// base64 binary, or a blob once `BlobModule` asked for one),
/// `websocketClosed` and `websocketFailed`.
class WebSocketModule final : public facebook::react::NativeWebSocketModuleCxxSpec<WebSocketModule> {
 public:
  WebSocketModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);
  ~WebSocketModule() override;

  void connect(
      facebook::jsi::Runtime &runtime,
      std::string url,
      std::optional<facebook::jsi::Array> protocols,
      facebook::jsi::Object options,
      double socketID);
  void send(facebook::jsi::Runtime &runtime, std::string message, double forSocketID);
  void sendBinary(facebook::jsi::Runtime &runtime, const std::string &base64String, double forSocketID);
  void ping(facebook::jsi::Runtime &runtime, double socketID);
  void close(facebook::jsi::Runtime &runtime, double code, std::string reason, double socketID);
  void addListener(facebook::jsi::Runtime &runtime, const std::string &eventName);
  void removeListeners(facebook::jsi::Runtime &runtime, double count);

 private:
  void failMissingSocket(int socketId) const;

  std::function<void(const std::string &, folly::dynamic)> emitDeviceEvent_;
};

} // namespace rntizen
