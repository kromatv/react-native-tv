#pragma once

#include "../platform/MainLoop.h"
#include "RemoteKeyMap.h"

#include <cstdint>
#include <functional>
#include <string_view>
#include <unordered_map>

namespace rnwebos {

/// One thing the remote did, as react-native-tvos reports it on tvOS: a press
/// is `eventType` with `keyAction` 1, at its release; a hold is `long<Type>`
/// with 0 when it starts and 1 when it ends; Back arrives once, at its release.
struct RemoteEvent {
  std::string_view eventType;
  int keyAction;
  bool isBack;
};

/// Turns key downs and ups into RemoteEvents, and tracks the keys held down
/// (docs/reference/webos-window-input.md, section 15). It never reports the
/// TV's repeats of a held key. Every key goes to the key filter first, when
/// there is one: a key the filter uses, down or up, is never a RemoteEvent,
/// and a release it uses ends the press it would have finished. Main thread
/// only: a hold starts on a main loop timer.
class RemotePresses final {
 public:
  using EventHandler = std::function<void(const RemoteEvent &event)>;
  /// Offered every key first, down (`isDown`) and up; true when it used the
  /// key. A focused text field reads keys this way.
  using KeyFilter = std::function<bool(const SDL_KeyboardEvent &key, bool isDown)>;

  explicit RemotePresses(EventHandler onEvent, KeyFilter keyFilter = {});
  ~RemotePresses();

  RemotePresses(const RemotePresses &) = delete;
  RemotePresses &operator=(const RemotePresses &) = delete;

  /// True when the filter or a remote key used it.
  bool keyDown(const SDL_KeyboardEvent &key);
  /// True when the filter used it or it released a key held down here.
  bool keyUp(const SDL_KeyboardEvent &key);
  /// Forgets every key held down, ending any hold, for when the app may never
  /// see their releases.
  void releaseAll();

 private:
  struct Press {
    RemoteKey key;
    std::uint64_t lastSeenMs;
    MainLoop::Id holdTimer;
    bool isHeld;
  };

  void track(std::uint32_t code, const SDL_KeyboardEvent &key, const RemoteKey &remoteKey);
  void finish(Press &press, bool released) const;
  void holdStarted(std::uint32_t code);

  EventHandler onEvent_;
  KeyFilter keyFilter_;
  std::unordered_map<std::uint32_t, Press> presses_;
};

} // namespace rnwebos
