#pragma once

#include <functional>
#include <memory>
#include <string>

namespace rnwebos {

/// What carries the calls of a LunaBus: LG's `libhelpers.so.2` on a TV, a fake
/// in tests. Every method may be called from any thread.
class LunaTransport {
 public:
  /// Called with the JSON text of one reply, on a thread of the transport's own.
  using OnMessage = std::function<void(std::string json)>;

  /// A call answered more than once: no reply reaches its handler once this is destroyed.
  class Subscription {
   public:
    virtual ~Subscription() = default;
  };

  virtual ~LunaTransport() = default;

  virtual bool isAvailable() const = 0;

  /// Sends a call that is answered once. False when the transport refuses it.
  virtual bool send(const std::string &uri, const std::string &payload, OnMessage onMessage) = 0;

  /// Sends a call that is answered until the result is destroyed. Null when the transport refuses it.
  virtual std::unique_ptr<Subscription> subscribe(
      const std::string &uri,
      const std::string &payload,
      OnMessage onMessage) = 0;
};

/// The TV's bus through `libhelpers.so.2`, opened at run time: unavailable
/// where the library is missing, as in the simulator, or cannot reach the bus.
std::unique_ptr<LunaTransport> makeLibHelpersTransport();

} // namespace rnwebos
