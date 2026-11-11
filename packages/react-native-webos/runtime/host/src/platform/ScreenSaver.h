#pragma once

#include "LunaBus.h"

#include <memory>
#include <optional>
#include <string>

namespace rnwebos {

/// Answers the TV when it asks whether the screen saver may start: yes, unless
/// SDL's screen saver is disabled, as while expo-keep-awake is active
/// (docs/reference/webos-window-input.md, section 15). Main thread only.
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

/// What the TV is told for `request`; nothing when it carries no timestamp to answer.
std::optional<folly::dynamic> screenSaverAnswer(
    const folly::dynamic &request,
    const std::string &clientName,
    bool allowed);

} // namespace rnwebos
