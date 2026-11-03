#pragma once

#include "ImeSession.h"

#include <Ecore_Input.h>
#include <Evas.h>

#include <memory>
#include <unordered_map>

namespace rntizen {

/// A text field that can take the keyboard.
class KeyboardClient : public ImeClient {
 public:
  /// A key pressed while the field has the keyboard; `repeat` for the
  /// keyboard's repeats of a key it already used. True when the field used
  /// it: its release then comes to the field too, and nothing else sees either.
  virtual bool keyDown(const Ecore_Event_Key &key, bool repeat) = 0;
  /// The release of a key the field used.
  virtual void keyUp(const Ecore_Event_Key &key) = 0;
  /// Another field took the keyboard, or the keyboard went away.
  virtual void keyboardLost() = 0;
};

/// Which text field on a canvas has the keyboard, and the input method it
/// types through. A key reaches that field, through the input method first,
/// before the remote handler sees it; a key either side used never reaches
/// the other, release included. Create, use and destroy it on the main thread,
/// before the views and after them.
class KeyboardFocus final {
 public:
  explicit KeyboardFocus(Evas *evas);
  ~KeyboardFocus();

  KeyboardFocus(const KeyboardFocus &) = delete;
  KeyboardFocus &operator=(const KeyboardFocus &) = delete;

  /// The keyboard focus of the canvas `evas`, or null when it has none.
  static KeyboardFocus *of(Evas *evas);

  /// Gives `client` the keyboard and starts the input method with `options`.
  void focus(KeyboardClient &client, const ImeOptions &options);
  /// Takes the keyboard back from `client`, if it has it.
  void blur(KeyboardClient &client);
  bool hasFocus(const KeyboardClient &client) const;

  /// Gives a key to the focused field. True when the field or its input
  /// method used it, so the remote handler must not.
  bool handleKey(const Ecore_Event_Key &key, bool isDown);

  /// The canvas's input method, created the first time a field takes the keyboard.
  ImeSession &ime();

 private:
  enum class Taker { InputMethod, Field };

  Evas *const evas_;
  std::unique_ptr<ImeSession> ime_;
  KeyboardClient *focused_{nullptr};
  std::unordered_map<unsigned int, Taker> takenKeys_;
};

} // namespace rntizen
