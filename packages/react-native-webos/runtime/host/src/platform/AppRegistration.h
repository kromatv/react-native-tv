#pragma once

#include "LunaBus.h"

#include <functional>
#include <memory>
#include <string>

namespace rnwebos {

/// The app's one registration with the TV's app manager, owed within three
/// seconds of its start, which then carries the app's lifecycle
/// (docs/reference/webos-tv-platform.md, section 3.3). Main thread only.
class AppRegistration final {
 public:
  struct Events {
    /// The app was launched again while it runs, with these parameters.
    std::function<void(const folly::dynamic &parameters, const std::string &reason)> relaunch;
    /// The app manager keeps the app alive in the background.
    std::function<void()> pause;
    /// The app must leave: the app manager may kill it one second later.
    std::function<void()> close;
  };

  /// Registers `appId` under lifecycle interface `interfaceVersion` (1 or 2),
  /// and under the other one when the app manager says the app declares that.
  /// Null when the bus is unavailable or refuses the call.
  static std::unique_ptr<AppRegistration> start(LunaBus &bus, std::string appId, int interfaceVersion, Events events);

  AppRegistration(const AppRegistration &) = delete;
  AppRegistration &operator=(const AppRegistration &) = delete;

 private:
  AppRegistration(LunaBus &bus, std::string appId, Events events);
  bool subscribe(int interfaceVersion);
  void onReply(const folly::dynamic &reply);

  LunaBus &bus_;
  const std::string appId_;
  const Events events_;
  int interfaceVersion_{0};
  bool registered_{false};
  bool triedOtherVersion_{false};
  std::unique_ptr<LunaBus::Subscription> subscription_;
};

} // namespace rnwebos
