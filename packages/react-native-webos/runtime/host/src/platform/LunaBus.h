#pragma once

#include <folly/dynamic.h>

#include <chrono>
#include <functional>
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
class LunaBus final {
 public:
  /// Called on the main thread with one reply, parsed.
  using Reply = std::function<void(const folly::dynamic &reply)>;

  /// A subscription, which ends when this is destroyed.
  class Subscription {
   public:
    virtual ~Subscription() = default;
  };

  static LunaBus &get();

  LunaBus(const LunaBus &) = delete;
  LunaBus &operator=(const LunaBus &) = delete;

  bool isAvailable() const;

  /// Sends one call. `onReply` gets its one reply; without it the reply is
  /// dropped. False when the bus is unavailable or refuses the call.
  bool call(const std::string &uri, const folly::dynamic &payload, Reply onReply = {});

  /// Sends one call and waits for its reply, at most `timeout`. Nothing when
  /// the bus is unavailable, refuses the call or does not answer in time.
  /// Never call it on the main thread once the app runs: it stalls the frame.
  std::optional<folly::dynamic> callAndWait(
      const std::string &uri,
      const folly::dynamic &payload,
      std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));

  /// Sends a call that is answered more than once (`"subscribe": true` in
  /// `payload` is the caller's to set). `onReply` gets every reply until the
  /// subscription is destroyed. Null when the bus is unavailable or refuses.
  std::unique_ptr<Subscription> subscribe(const std::string &uri, const folly::dynamic &payload, Reply onReply);

 private:
  LunaBus();
  ~LunaBus() = default;

  struct Library;
  const std::unique_ptr<Library> library_;
};

} // namespace rnwebos
