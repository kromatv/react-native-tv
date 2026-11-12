#include "WebSocketSockets.h"

namespace rntizen {

WebSocketSockets &WebSocketSockets::shared() {
  static WebSocketSockets sockets;
  return sockets;
}

void WebSocketSockets::add(int socketId, std::shared_ptr<WebSocketConnection> connection) {
  std::shared_ptr<WebSocketConnection> replaced;
  std::lock_guard lock(mutex_);
  auto &slot = connections_[socketId];
  replaced = std::exchange(slot, std::move(connection));
}

std::shared_ptr<WebSocketConnection> WebSocketSockets::find(int socketId) const {
  std::lock_guard lock(mutex_);
  auto found = connections_.find(socketId);
  return found == connections_.end() ? nullptr : found->second;
}

void WebSocketSockets::forget(int socketId) {
  std::shared_ptr<WebSocketConnection> forgotten;
  std::lock_guard lock(mutex_);
  if (auto found = connections_.find(socketId); found != connections_.end()) {
    forgotten = std::move(found->second);
    connections_.erase(found);
  }
  blobSockets_.erase(socketId);
}

void WebSocketSockets::forgetAll() {
  std::map<int, std::shared_ptr<WebSocketConnection>> forgotten;
  std::lock_guard lock(mutex_);
  forgotten.swap(connections_);
  blobSockets_.clear();
}

void WebSocketSockets::setDeliversBlobs(int socketId, bool deliversBlobs) {
  std::lock_guard lock(mutex_);
  if (deliversBlobs) {
    blobSockets_.insert(socketId);
  } else {
    blobSockets_.erase(socketId);
  }
}

bool WebSocketSockets::deliversBlobs(int socketId) const {
  std::lock_guard lock(mutex_);
  return blobSockets_.contains(socketId);
}

} // namespace rntizen
