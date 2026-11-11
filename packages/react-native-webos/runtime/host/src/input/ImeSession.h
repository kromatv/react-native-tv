#pragma once

#include "../views/PixelGeometry.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>

#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace rnwebos {

/// What the TV's keyboard asks of the text field it types into.
class ImeClient {
 public:
  virtual ~ImeClient() = default;

  /// Text the user finished typing, to replace the composition or the selection.
  virtual void imeCommit(std::string_view text) = 0;
  /// Text still being composed, drawn at the caret until it is committed, the
  /// caret `cursor` characters into it. An empty `text` ends the composition.
  virtual void imePreedit(std::string_view text, int cursor) = 0;
  /// The on-screen keyboard closed while the field still had the keyboard.
  virtual void imePanelHidden() = 0;
};

/// The TextInput props that choose a keyboard.
struct KeyboardProps {
  std::string_view keyboardType{};
  std::string_view returnKeyType{};
  std::string_view autoCapitalize{};
  bool autoCorrect{false};
  bool secure{false};
  bool multiline{false};
  bool showOnFocus{true};
};

/// How SDL starts text input for one field: the properties of
/// `SDL_StartTextInputWithProperties` (docs/reference/webos-window-input.md,
/// sections 10 and 15).
struct ImeOptions {
  SDL_TextInputType type{SDL_TEXTINPUT_TYPE_TEXT};
  SDL_Capitalization capitalization{SDL_CAPITALIZE_SENTENCES};
  bool autocorrect{false};
  bool multiline{false};
  /// What the props ask that SDL's text input cannot say, one sentence each.
  std::vector<std::string> unsupported{};
};

ImeOptions imeOptionsFor(const KeyboardProps &props);

/// SDL's text input on the app's window, the way a native webOS app reaches
/// the TV's on-screen keyboard: starting it raises the keyboard, stopping it
/// is the only way to lower it. One field edits through it at a time. Main
/// thread only.
class ImeSession final {
 public:
  /// `window` may be null, as for a scene without a window: the session then
  /// keeps its own state and calls nothing.
  explicit ImeSession(SDL_Window *window);
  ~ImeSession();

  ImeSession(const ImeSession &) = delete;
  ImeSession &operator=(const ImeSession &) = delete;

  /// Whether the device has an on-screen keyboard: SDL says so, or one showed.
  bool isAvailable() const;
  /// Starts editing `client` with `options`, which raises the keyboard.
  void begin(ImeClient &client, const ImeOptions &options);
  /// Ends the edit: stops text input, which lowers the keyboard.
  void end();
  /// Gives the field being edited what a text input event of SDL says: text
  /// committed, text being composed, the keyboard shown or hidden. False for
  /// any other event.
  bool handleEvent(const SDL_Event &event);
  /// Raises the keyboard again for the field being edited.
  void showPanel();
  /// Lowers the keyboard and keeps the field: no text arrives until `showPanel`.
  void hidePanel();
  bool isPanelShown() const;
  /// Whether text input is started, so a key that types sends its text.
  bool isActive() const;
  /// Tells the keyboard where the caret is, in window pixels, inside `line`:
  /// the field's width at the caret's height.
  void caretMoved(const PixelRect &line, int caretX);
  /// Drops the composition, for text that changed under it.
  void reset();

 private:
  void start();
  void stop();
  void compositionEmptied();

  SDL_Window *const window_;
  ImeClient *client_{nullptr};
  ImeOptions options_{};
  std::set<std::string, std::less<>> reported_;
  std::shared_ptr<bool> emptiedComposition_;
  bool active_{false};
  bool panelShown_{false};
  bool panelSeen_{false};
};

} // namespace rnwebos
