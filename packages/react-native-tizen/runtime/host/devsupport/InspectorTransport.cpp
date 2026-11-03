#include "InspectorTransport.h"

#include "network/WebSocketConnection.h"
#include "threading/MainThread.h"

#include <Ecore.h>

#include <optional>

namespace rntizen {

namespace {

using facebook::react::jsinspector_modern::IWebSocket;
using facebook::react::jsinspector_modern::IWebSocketDelegate;

void onMainThread(std::weak_ptr<IWebSocketDelegate> delegate, std::function<void(IWebSocketDelegate &)> event) {
  runOnMainThread([delegate = std::move(delegate), event = std::move(event)] {
    if (auto strong = delegate.lock()) event(*strong);
  });
}

class InspectorSocket final : public IWebSocket {
 public:
  InspectorSocket(const std::string &url, const std::weak_ptr<IWebSocketDelegate> &delegate)
      : connection_(
            WebSocketRequest{.url = url, .protocols = {}, .headers = {}},
            WebSocketListener{
                .onOpen = [delegate](const std::string &) { onMainThread(delegate, [](auto &d) { d.didOpen(); }); },
                .onMessage =
                    [delegate](std::string data, bool) {
                      onMainThread(delegate, [data = std::move(data)](auto &d) { d.didReceiveMessage(data); });
                    },
                .onClosed =
                    [delegate](int, const std::string &) { onMainThread(delegate, [](auto &d) { d.didClose(); }); },
                .onFailed =
                    [delegate](const std::string &message) {
                      onMainThread(delegate, [message](auto &d) { d.didFailWithError(std::nullopt, message); });
                    },
            }) {}

  void send(std::string_view message) override {
    connection_.send(WebSocketOpcode::Text, std::string(message));
  }

 private:
  WebSocketConnection connection_;
};

Eina_Bool runCallback(void *data) {
  std::unique_ptr<std::function<void()>> callback(static_cast<std::function<void()> *>(data));
  (*callback)();
  return ECORE_CALLBACK_CANCEL;
}

} // namespace

std::unique_ptr<IWebSocket> InspectorTransport::connectWebSocket(
    const std::string &url,
    std::weak_ptr<IWebSocketDelegate> delegate) {
  return std::make_unique<InspectorSocket>(url, delegate);
}

void InspectorTransport::scheduleCallback(std::function<void()> callback, std::chrono::milliseconds delay) {
  runOnMainThread([callback = std::move(callback), delay]() mutable {
    const auto seconds = std::chrono::duration<double>(delay).count();
    ecore_timer_add(seconds, runCallback, std::make_unique<std::function<void()>>(std::move(callback)).release());
  });
}

} // namespace rntizen
