#pragma once

#include "WebSocketFrame.h"

#include <atomic>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace rntizen {

/// What JavaScript's `new WebSocket(url, protocols, {headers})` asks for.
struct WebSocketRequest {
  std::string url;
  std::vector<std::string> protocols;
  std::vector<std::pair<std::string, std::string>> headers;
};

/// What a connection reports, on its own thread and in order: `onOpen` once,
/// the messages, then `onClosed` or `onFailed`, once. A connection that never
/// opened reports `onFailed` only.
struct WebSocketListener {
  std::function<void(std::string protocol)> onOpen;
  std::function<void(std::string data, bool binary)> onMessage;
  std::function<void(int code, std::string reason)> onClosed;
  std::function<void(std::string message)> onFailed;
};

/// One client WebSocket (RFC 6455) on its own thread. libcurl opens the TCP or
/// TLS connection with `CURLOPT_CONNECT_ONLY` and this class speaks the
/// protocol over it, because the Tizen 9 rootstrap's libcurl is built without
/// WebSocket support. Every method is thread-safe and returns at once; the
/// destructor stops the thread without reporting anything more.
class WebSocketConnection final {
 public:
  WebSocketConnection(WebSocketRequest request, WebSocketListener listener);
  ~WebSocketConnection();

  WebSocketConnection(const WebSocketConnection &) = delete;
  WebSocketConnection &operator=(const WebSocketConnection &) = delete;

  /// Sent once the connection is open, in the order given.
  void send(WebSocketOpcode opcode, std::string payload);
  /// Starts the closing handshake. `onClosed` follows the server's Close, or
  /// a few seconds without one.
  void close(int code, std::string_view reason);

 private:
  struct Command {
    WebSocketOpcode opcode;
    std::string payload;
  };
  struct Incoming;
  struct Closing;
  class Session;

  void run();
  std::string handshake(Session &session, std::string &inbox, std::string_view key) const;
  void exchange(Session &session, Incoming &incoming);
  void queueCommands(Session &session, std::optional<Closing> &closing);
  bool readFrames(Session &session, Incoming &incoming, const std::optional<Closing> &closing) const;
  void onServerClose(Session &session, const WebSocketFrame &frame, const std::optional<Closing> &closing) const;
  void onMessageFrame(Incoming &incoming, WebSocketFrame &frame) const;
  void wake() const;
  std::deque<Command> takeCommands();
  void reportMessage(std::string data, bool binary) const;
  void reportClosed(int code, std::string reason) const;

  const WebSocketRequest request_;
  const WebSocketListener listener_;
  const int wakeFd_;
  std::atomic<bool> stopping_{false};
  std::mutex mutex_;
  std::deque<Command> commands_;
  std::jthread thread_;
};

} // namespace rntizen
