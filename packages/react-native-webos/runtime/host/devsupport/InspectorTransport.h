#pragma once

#include <jsinspector-modern/InspectorPackagerConnection.h>

#include <chrono>
#include <functional>
#include <memory>
#include <string>

namespace rntizen {

/// What jsinspector-modern's InspectorPackagerConnection needs from a
/// platform: WebSockets to the dev server's inspector proxy, and delayed
/// callbacks. The main loop is the inspector queue, as the UI thread is on
/// Android: every socket event and callback runs there.
class InspectorTransport final : public facebook::react::jsinspector_modern::InspectorPackagerConnectionDelegate {
 public:
  std::unique_ptr<facebook::react::jsinspector_modern::IWebSocket> connectWebSocket(
      const std::string &url,
      std::weak_ptr<facebook::react::jsinspector_modern::IWebSocketDelegate> delegate) override;

  void scheduleCallback(std::function<void()> callback, std::chrono::milliseconds delay) override;
};

} // namespace rntizen
