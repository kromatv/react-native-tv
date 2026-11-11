#pragma once

#include "LunaBus.h"

#include <memory>
#include <optional>
#include <string>

namespace rnwebos {

/// The app's say over the TV's screen saver. The TV asks every registered
/// client before it starts the saver, and this answers yes unless SDL's screen
/// saver is disabled (`SDL_DisableScreenSaver`), as while expo-keep-awake is
/// active. SDL answers these itself only when it registers the app, which the
/// host does in its place (docs/reference/webos-window-input.md, section 15).
/// Main thread only.
class ScreenSaver final {
 public:
  /// Null when the bus is unavailable or refuses the subscription.
  static std::unique_ptr<ScreenSaver> watch(LunaBus &bus, const std::string &appId);

  ScreenSaver(const ScreenSaver &) = delete;
  ScreenSaver &operator=(const ScreenSaver &) = delete;

 private:
  ScreenSaver(LunaBus &bus, std::string clientName);
  void answer(const folly::dynamic &request) const;

  LunaBus &bus_;
  const std::string clientName_;
  std::unique_ptr<LunaBus::Subscription> requests_;
};

/// What the TV is told for the request `request` of client `clientName`:
/// nothing when the request carries no timestamp to answer.
std::optional<folly::dynamic> screenSaverAnswer(
    const folly::dynamic &request,
    const std::string &clientName,
    bool allowed);

} // namespace rnwebos
