#pragma once

#include "../../network/WebSocketConnection.h"

#include <map>
#include <memory>
#include <mutex>
#include <set>

namespace rntizen {

/// Every WebSocket the app has open, by the id JavaScript numbered it with.
/// `WebSocketModule` opens and forgets them; `BlobModule` sends blobs on them
/// and asks for their binary messages as blobs, as Android's `BlobModule`
/// reaches into its `WebSocketModule`. One per process, as a TV app runs one
/// React Native instance. Thread-safe, but a connection must never forget
/// itself: forgetting the last reference joins its thread.
class WebSocketSockets final {
 public:
  static WebSocketSockets &shared();

  void add(int socketId, std::shared_ptr<WebSocketConnection> connection);
  std::shared_ptr<WebSocketConnection> find(int socketId) const;
  void forget(int socketId);
  void forgetAll();

  void setDeliversBlobs(int socketId, bool deliversBlobs);
  bool deliversBlobs(int socketId) const;

 private:
  mutable std::mutex mutex_;
  std::map<int, std::shared_ptr<WebSocketConnection>> connections_;
  std::set<int> blobSockets_;
};

} // namespace rntizen
