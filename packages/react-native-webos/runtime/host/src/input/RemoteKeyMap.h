#pragma once

#include <SDL3/SDL_events.h>

#include <optional>
#include <string_view>

namespace rnwebos {

/// What a remote key means to React Native: a `useTVEventHandler` event type
/// (`up`, `select`, `playPause`...) and, for the keys react-native-tvos reports
/// held, the event a hold sends (`longSelect`...); or the Back button, which
/// goes through `BackHandler` instead.
struct RemoteKey {
  std::string_view eventType;
  std::string_view longEventType;
  bool isBack;

  bool hasLongPress() const {
    return !longEventType.empty();
  }
};

/// Maps a key SDL reports to the event a react-native-tvos app expects;
/// nullopt for a key apps do not see. A remote key SDL has no scancode for is
/// matched on `raw`, the Linux key code webOS sent. Table in
/// docs/reference/webos-window-input.md, section 15.
std::optional<RemoteKey> remoteKeyFor(SDL_Scancode scancode, Uint16 raw);

/// The scancode `rn-webos simulator key <name>` presses for a remote key name
/// (`Back`, `Red`, `PlayPause`...), or SDL_SCANCODE_UNKNOWN.
SDL_Scancode scancodeForKeyName(std::string_view name);

} // namespace rnwebos
