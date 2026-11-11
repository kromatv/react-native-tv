#pragma once

#include <optional>
#include <span>
#include <string_view>

namespace rntizen {

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

/// Maps an EFL key symbol (`Ecore_Event_Key::key`: `Up`, `Return`, `XF86Back`,
/// `XF86AudioNext`...) to the event a react-native-tvos app expects; nullopt for
/// a key apps do not see. Table in docs/reference/tizen-input.md, section 12.
std::optional<RemoteKey> remoteKeyFor(std::string_view keySymbol);

/// The keys a Samsung TV delivers to an app only once it grabs them: Back, the
/// media keys, the chapter, guide, Tools, caption, info and color keys.
std::span<const std::string_view> grabbedKeyNames();

} // namespace rntizen
