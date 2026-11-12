#pragma once

#include "LunaConnection.h"
#include "LunaTransport.h"

#include <chrono>
#include <memory>
#include <optional>
#include <string>

namespace rnwebos {

/// The TV's Luna bus, where the app manager, the settings, the media pipeline
/// and every other webOS service answer JSON calls. Calls go through LG's
/// `libhelpers.so.2`, which every webOS TV release has and which keeps its own
/// connection and thread (docs/reference/webos-tv-platform.md, section 4): the
/// library is opened at run time, and where it is missing, as in the
/// simulator, the bus is unavailable and every call says so. Any thread.
class LunaBus final : public LunaConnection {
 public:
  /// The reply of one call, which may still be on its way.
  class PendingReply {
   public:
    virtual ~PendingReply() = default;
    /// The reply, waited for until `deadline` at most. Nothing when it has not come by then.
    virtual std::optional<folly::dynamic> waitUntil(std::chrono::steady_clock::time_point deadline) = 0;
  };

  static LunaBus &get();

  /// A bus over another transport than the TV's, for tests.
  explicit LunaBus(std::unique_ptr<LunaTransport> transport);
  ~LunaBus() override;

  LunaBus(const LunaBus &) = delete;
  LunaBus &operator=(const LunaBus &) = delete;

  bool isAvailable() const override;

  bool call(const std::string &uri, const folly::dynamic &payload, Reply onReply = {}) override;

  /// Sends one call and returns at once: the reply is kept for whoever waits
  /// for it, on any thread. Null when the bus is unavailable or refuses the call.
  std::shared_ptr<PendingReply> ask(const std::string &uri, const folly::dynamic &payload);

  /// Sends one call and waits for its reply, at most `timeout`. Nothing when
  /// the bus is unavailable, refuses the call or does not answer in time.
  /// Never call it on the main thread once the app runs: it stalls the frame.
  std::optional<folly::dynamic> callAndWait(
      const std::string &uri,
      const folly::dynamic &payload,
      std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));

  std::unique_ptr<Subscription> subscribe(
      const std::string &uri, const folly::dynamic &payload, Reply onReply) override;

 private:
  const std::unique_ptr<LunaTransport> transport_;
};

} // namespace rnwebos
