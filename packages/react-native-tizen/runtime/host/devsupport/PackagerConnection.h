#pragma once

#include "network/WebSocketConnection.h"

#include <Ecore.h>

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace rntizen {

/// The socket an app in development keeps open to its dev server's
/// `/message`, speaking protocol version 2: the server broadcasts `reload`
/// (`r` in `expo start`) and `devMenu` (`m`). While the server is away it
/// tries again every 2 s, as Android's ReconnectingWebSocket does. Create, use
/// and destroy it on the main thread; the handlers run there.
class PackagerConnection final {
 public:
  struct Handlers {
    std::function<void()> reload;
    std::function<void()> devMenu;
  };

  PackagerConnection(std::string url, Handlers handlers);
  ~PackagerConnection();

  PackagerConnection(const PackagerConnection &) = delete;
  PackagerConnection &operator=(const PackagerConnection &) = delete;

 private:
  void open();
  void retryLater();
  void receive(std::string_view text);
  static Eina_Bool retry(void *data);

  const std::string url_;
  const Handlers handlers_;
  std::unique_ptr<WebSocketConnection> socket_;
  Ecore_Timer *retryTimer_{nullptr};
  bool reportedFailure_{false};
  std::shared_ptr<PackagerConnection *> lifetime_;
};

} // namespace rntizen
