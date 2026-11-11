#pragma once

#include "RemoteKeyMap.h"

#include <Ecore.h>
#include <Ecore_Input.h>

#include <functional>
#include <string_view>
#include <unordered_map>

namespace rntizen {

/// One thing the remote did, as react-native-tvos reports it on tvOS: a press
/// is `eventType` with `keyAction` 1, at its release; a hold is `long<Type>`
/// with 0 when it starts and 1 when it ends; Back arrives once, at its release.
struct RemoteEvent {
  std::string_view eventType;
  int keyAction;
  bool isBack;
};

/// Turns key downs and ups into RemoteEvents, and tracks the keys held down
/// (docs/reference/tizen-input.md, section 12). It never reports the TV's
/// repeats of a held key. Every key goes to the key filter first, when there
/// is one: a key the filter uses, down or up, is never a RemoteEvent, and a
/// release it uses ends the press it would have finished. Main thread only: a
/// hold starts on an Ecore timer.
class RemotePresses final {
 public:
  using EventHandler = std::function<void(const RemoteEvent &event)>;
  /// Offered every key first, down (`isDown`) and up; true when it used the
  /// key. A focused text field reads keys this way.
  using KeyFilter = std::function<bool(const Ecore_Event_Key &key, bool isDown)>;

  explicit RemotePresses(EventHandler onEvent, KeyFilter keyFilter = {});
  ~RemotePresses();

  RemotePresses(const RemotePresses &) = delete;
  RemotePresses &operator=(const RemotePresses &) = delete;

  /// True when the filter or a remote key used it.
  bool keyDown(const Ecore_Event_Key &key);
  /// True when the filter used it or it released a key held down here.
  bool keyUp(const Ecore_Event_Key &key);
  /// Forgets every key held down, ending any hold, for when the app may never
  /// see their releases.
  void releaseAll();

 private:
  struct Press {
    RemotePresses *owner;
    RemoteKey key;
    unsigned int lastSeen;
    Ecore_Timer *holdTimer;
    bool isHeld;
  };

  static Eina_Bool onHoldStarted(void *press);

  void track(const Ecore_Event_Key &key, const RemoteKey &remoteKey);
  void finish(Press &press, bool released);

  EventHandler onEvent_;
  KeyFilter keyFilter_;
  std::unordered_map<unsigned int, Press> presses_;
};

} // namespace rntizen
