#include "WebSocketModule.h"

#include "../PlatformModuleRegistry.h"
#include "../blob/BlobStore.h"
#include "WebSocketSockets.h"

#include "../../logging/Log.h"
#include "../../network/Base64.h"

#include <algorithm>
#include <stdexcept>

namespace rntizen {

namespace {

using facebook::jsi::Runtime;

std::vector<std::string> protocolList(Runtime &runtime, const std::optional<facebook::jsi::Array> &protocols) {
  std::vector<std::string> list;
  if (!protocols) return list;
  for (std::size_t index = 0; index < protocols->size(runtime); ++index) {
    auto value = protocols->getValueAtIndex(runtime, index);
    if (!value.isString()) continue;
    auto protocol = value.getString(runtime).utf8(runtime);
    const auto first = protocol.find_first_not_of(" \t");
    if (first == std::string::npos || std::ranges::count(protocol, ',') > 0) continue;
    list.push_back(protocol.substr(first, protocol.find_last_not_of(" \t") - first + 1));
  }
  return list;
}

std::vector<std::pair<std::string, std::string>> headerList(Runtime &runtime, const facebook::jsi::Object &options) {
  std::vector<std::pair<std::string, std::string>> list;
  auto headers = options.getProperty(runtime, "headers");
  if (!headers.isObject()) return list;
  auto object = headers.getObject(runtime);
  auto names = object.getPropertyNames(runtime);
  for (std::size_t index = 0; index < names.size(runtime); ++index) {
    auto name = names.getValueAtIndex(runtime, index).getString(runtime).utf8(runtime);
    auto value = object.getProperty(runtime, name.c_str());
    if (value.isString()) {
      list.emplace_back(name, value.getString(runtime).utf8(runtime));
    } else {
      log(LogLevel::Warning, "WebSocket: ignoring header " + name + ", whose value is not a string");
    }
  }
  return list;
}

folly::dynamic messageEvent(int socketId, std::string data, bool binary) {
  if (!binary) return folly::dynamic::object("id", socketId)("type", "text")("data", std::move(data));
  if (WebSocketSockets::shared().deliversBlobs(socketId)) {
    const auto size = static_cast<int64_t>(data.size());
    auto blobId = BlobStore::shared().store(std::move(data));
    return folly::dynamic::object("id", socketId)("type", "blob")(
        "data", folly::dynamic::object("blobId", std::move(blobId))("offset", 0)("size", size));
  }
  return folly::dynamic::object("id", socketId)("type", "binary")("data", encodeBase64(data));
}

} // namespace

WebSocketModule::WebSocketModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context)
    : NativeWebSocketModuleCxxSpec(std::move(jsInvoker)), emitDeviceEvent_(context.emitDeviceEvent) {}

WebSocketModule::~WebSocketModule() {
  WebSocketSockets::shared().forgetAll();
}

void WebSocketModule::connect(
    Runtime &runtime,
    std::string url,
    std::optional<facebook::jsi::Array> protocols,
    facebook::jsi::Object options,
    double socketID) {
  const auto id = static_cast<int>(socketID);
  auto emit = emitDeviceEvent_;
  auto forget = [invoker = jsInvoker_, id] {
    invoker->invokeAsync([id] { WebSocketSockets::shared().forget(id); });
  };
  WebSocketListener listener{
      .onOpen =
          [emit, id](std::string protocol) {
            emit("websocketOpen", folly::dynamic::object("id", id)("protocol", std::move(protocol)));
          },
      .onMessage =
          [emit, id](std::string data, bool binary) {
            emit("websocketMessage", messageEvent(id, std::move(data), binary));
          },
      .onClosed =
          [emit, id, forget](int code, std::string reason) {
            emit("websocketClosed", folly::dynamic::object("id", id)("code", code)("reason", std::move(reason)));
            forget();
          },
      .onFailed =
          [emit, id, forget](std::string message) {
            emit("websocketFailed", folly::dynamic::object("id", id)("message", std::move(message)));
            forget();
          },
  };
  WebSocketSockets::shared().add(
      id,
      std::make_shared<WebSocketConnection>(
          WebSocketRequest{
              .url = std::move(url),
              .protocols = protocolList(runtime, protocols),
              .headers = headerList(runtime, options),
          },
          std::move(listener)));
}

void WebSocketModule::failMissingSocket(int socketId) const {
  emitDeviceEvent_("websocketFailed", folly::dynamic::object("id", socketId)("message", "client is null"));
  emitDeviceEvent_("websocketClosed", folly::dynamic::object("id", socketId)("code", 0)("reason", "client is null"));
}

void WebSocketModule::send(Runtime &, std::string message, double forSocketID) {
  const auto id = static_cast<int>(forSocketID);
  auto connection = WebSocketSockets::shared().find(id);
  if (!connection) return failMissingSocket(id);
  connection->send(WebSocketOpcode::Text, std::move(message));
}

void WebSocketModule::sendBinary(Runtime &, const std::string &base64String, double forSocketID) {
  const auto id = static_cast<int>(forSocketID);
  auto connection = WebSocketSockets::shared().find(id);
  if (!connection) return failMissingSocket(id);
  try {
    connection->send(WebSocketOpcode::Binary, decodeBase64(base64String));
  } catch (const std::invalid_argument &error) {
    emitDeviceEvent_("websocketFailed", folly::dynamic::object("id", id)("message", error.what()));
  }
}

void WebSocketModule::ping(Runtime &, double socketID) {
  const auto id = static_cast<int>(socketID);
  auto connection = WebSocketSockets::shared().find(id);
  if (!connection) return failMissingSocket(id);
  connection->send(WebSocketOpcode::Ping, {});
}

void WebSocketModule::close(Runtime &, double code, const std::string &reason, double socketID) {
  if (auto connection = WebSocketSockets::shared().find(static_cast<int>(socketID))) {
    connection->close(static_cast<int>(code), reason);
  }
}

void WebSocketModule::addListener(Runtime &, const std::string &) {}

void WebSocketModule::removeListeners(Runtime &, double) {}

RN_TIZEN_PLATFORM_MODULE("WebSocketModule", WebSocketModule);

} // namespace rntizen
