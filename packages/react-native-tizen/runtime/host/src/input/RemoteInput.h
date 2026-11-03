#pragma once

#include "RemoteKeyMap.h"

#include <Ecore.h>
#include <Ecore_Input.h>
#include <Evas.h>

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

/// Listens to the remote on the app's window. It grabs the keys a TV gives an
/// app only on request, never reports the TV's repeats of a held key, and turns
/// key downs and ups into RemoteEvents (docs/reference/tizen-input.md, section
/// 12). Every key goes to the key filter first, when there is one: a key the
/// filter uses, down or up, is never a RemoteEvent. The grabs last as long as
/// the window: destroy it with the window. Create, use and destroy it on the
/// main thread.
class RemoteInput final {
 public:
  using EventHandler = std::function<void(const RemoteEvent &event)>;
  /// Offered every key of the window first, down (`isDown`) and up; true
  /// when it used the key. A focused text field reads keys this way.
  using KeyFilter = std::function<bool(const Ecore_Event_Key &key, bool isDown)>;

  RemoteInput(Evas_Object *window, EventHandler onEvent, KeyFilter keyFilter = {});
  ~RemoteInput();

  RemoteInput(const RemoteInput &) = delete;
  RemoteInput &operator=(const RemoteInput &) = delete;

  /// Forgets every key held down, ending any hold, for when the app may never
  /// see their releases: it was paused or lost the focus.
  void releaseAll();

 private:
  struct Press {
    RemoteInput *owner;
    RemoteKey key;
    unsigned int lastSeen;
    Ecore_Timer *holdTimer;
    bool isHeld;
  };

  static Eina_Bool onKeyDown(void *self, int type, void *event);
  static Eina_Bool onKeyUp(void *self, int type, void *event);
  static Eina_Bool onHoldStarted(void *press);
  static void onFocusOut(void *self, Evas_Object *window, void *info);

  bool isForWindow(const Ecore_Event_Key &key) const;
  void keyDown(const Ecore_Event_Key &key, const RemoteKey &remoteKey);
  bool keyUp(const Ecore_Event_Key &key);
  void finish(Press &press, bool released);

  Evas_Object *window_;
  Ecore_Window windowId_;
  EventHandler onEvent_;
  KeyFilter keyFilter_;
  std::unordered_map<unsigned int, Press> presses_;
  Ecore_Event_Handler *keyDown_;
  Ecore_Event_Handler *keyUp_;
};

} // namespace rntizen
