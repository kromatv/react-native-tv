#pragma once

#include "../render/Scene.h"
#include "ImeSession.h"

#include <SDL3/SDL_events.h>

#include <cstdint>
#include <memory>
#include <unordered_set>

namespace rnwebos {

/// A text field that can take the keyboard.
class KeyboardClient : public ImeClient {
 public:
  /// A key pressed while the field has the keyboard; `repeat` for the
  /// keyboard's repeats of a key it already used. True when the field used
  /// it: its release then comes to the field too, and nothing else sees either.
  virtual bool keyDown(const SDL_KeyboardEvent &key, bool repeat) = 0;
  /// The release of a key the field used.
  virtual void keyUp(const SDL_KeyboardEvent &key) = 0;
  /// Another field took the keyboard, or the keyboard went away.
  virtual void keyboardLost() = 0;
};

/// Which text field of a scene has the keyboard, and the text input session
/// it types through. A key reaches that field before the remote handler sees
/// it; a key the field used never reaches the remote handler, release
/// included. Create, use and destroy it on the main thread, before the views
/// and after them.
class KeyboardFocus final {
 public:
  explicit KeyboardFocus(Scene &scene);
  ~KeyboardFocus();

  KeyboardFocus(const KeyboardFocus &) = delete;
  KeyboardFocus &operator=(const KeyboardFocus &) = delete;

  /// The keyboard focus of `scene`, or null when it has none.
  static KeyboardFocus *of(const Scene &scene);

  /// Gives `client` the keyboard and starts text input with `options`.
  void focus(KeyboardClient &client, const ImeOptions &options);
  /// Takes the keyboard back from `client`, if it has it.
  void blur(KeyboardClient &client);
  bool hasFocus(const KeyboardClient &client) const;

  /// Gives a key to the focused field. True when the field used it, so the
  /// remote handler must not.
  bool handleKey(const SDL_KeyboardEvent &key, bool isDown);
  /// Gives the focused field one of SDL's text input events (text, a
  /// composition, the on-screen keyboard shown or hidden). False for any other.
  bool handleTextEvent(const SDL_Event &event);
  /// Forgets the keys a field holds down, for when the app may never see
  /// their releases: it was paused, or it reloads.
  void releaseKeys();

  /// The scene's text input session, created the first time a field takes the keyboard.
  ImeSession &ime();

 private:
  Scene &scene_;
  std::unique_ptr<ImeSession> ime_;
  KeyboardClient *focused_{nullptr};
  std::unordered_set<std::uint32_t> takenKeys_;
};

} // namespace rnwebos
