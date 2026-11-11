#pragma once

#include "RemotePresses.h"

#include <Ecore.h>
#include <Ecore_Input.h>
#include <Evas.h>

namespace rntizen {

/// Listens to the remote on the app's window: it grabs the keys a TV gives an
/// app only on request and hands the window's key downs and ups to
/// RemotePresses, which turns them into RemoteEvents. The grabs last as long
/// as the window: destroy it with the window. Create, use and destroy it on
/// the main thread.
class RemoteInput final {
 public:
  RemoteInput(Evas_Object *window, RemotePresses::EventHandler onEvent, RemotePresses::KeyFilter keyFilter = {});
  ~RemoteInput();

  RemoteInput(const RemoteInput &) = delete;
  RemoteInput &operator=(const RemoteInput &) = delete;

  /// Forgets every key held down, ending any hold, for when the app may never
  /// see their releases: it was paused or lost the focus.
  void releaseAll();

 private:
  static Eina_Bool onKeyDown(void *self, int type, void *event);
  static Eina_Bool onKeyUp(void *self, int type, void *event);
  static void onFocusOut(void *self, Evas_Object *window, void *info);

  bool isForWindow(const Ecore_Event_Key &key) const;

  Evas_Object *window_;
  Ecore_Window windowId_;
  RemotePresses presses_;
  Ecore_Event_Handler *keyDown_;
  Ecore_Event_Handler *keyUp_;
};

} // namespace rntizen
