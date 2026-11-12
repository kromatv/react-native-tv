#pragma once

#include <folly/dynamic.h>

#include <functional>
#include <memory>
#include <string>

namespace rnwebos {

/// Something that takes Luna calls: the app's own connection to the bus
/// (LunaBus), one registered under another name (LunaClient), or the fake a
/// test replays a service's JSON from. Any thread.
class LunaConnection {
 public:
  /// Called on the main thread with one reply, parsed.
  using Reply = std::function<void(const folly::dynamic &reply)>;

  /// A subscription, which ends when this is destroyed.
  class Subscription {
   public:
    virtual ~Subscription() = default;
  };

  virtual ~LunaConnection() = default;

  virtual bool isAvailable() const = 0;

  /// Sends one call. `onReply` gets its one reply; without it the reply is
  /// dropped. False when the bus is unavailable or refuses the call.
  virtual bool call(const std::string &uri, const folly::dynamic &payload, Reply onReply = {}) = 0;

  /// Sends a call that is answered more than once (`"subscribe": true` in
  /// `payload` is the caller's to set). `onReply` gets every reply until the
  /// subscription is destroyed. Null when the bus is unavailable or refuses.
  virtual std::unique_ptr<Subscription> subscribe(
      const std::string &uri, const folly::dynamic &payload, Reply onReply) = 0;
};

} // namespace rnwebos
