#pragma once

#include <Ecore_IMF.h>
#include <Ecore_Input.h>
#include <Evas.h>

#include <string>
#include <string_view>

namespace rntizen {

/// What an input method asks of the text field it types into. Offsets and
/// lengths count characters (Unicode code points).
class ImeClient {
 public:
  virtual ~ImeClient() = default;

  /// Text the user finished typing, to replace the selection.
  virtual void imeCommit(std::string_view text) = 0;
  /// Text still being composed, drawn at the caret until it is committed.
  /// `cursor` is inside `text`.
  virtual void imePreedit(std::string_view text, int cursor) = 0;
  /// Deletes `count` characters starting `offset` characters from the caret.
  virtual void imeDeleteSurrounding(int offset, int count) = 0;
  /// The field's text, masked when it is a password, and the caret in it.
  virtual std::string imeSurrounding(int &cursor) const = 0;
  /// The on-screen keyboard closed while the field still had the keyboard.
  virtual void imePanelHidden() = 0;
};

/// How the on-screen keyboard looks for one field (`ecore_imf_context_*`
/// options, docs/reference/tizen-input.md, section 10).
struct ImeOptions {
  Ecore_IMF_Input_Panel_Layout layout{ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL};
  int layoutVariation{0};
  Ecore_IMF_Input_Panel_Return_Key_Type returnKey{ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT};
  Ecore_IMF_Autocapital_Type autocapital{ECORE_IMF_AUTOCAPITAL_TYPE_SENTENCE};
  bool prediction{true};
  bool secure{false};
  bool multiline{false};
  /// Whether the panel opens with the edit (`showSoftInputOnFocus`).
  bool showPanel{true};
};

/// The options for React Native's TextInput props: `keyboardType`,
/// `returnKeyType`, `autoCapitalize`, `autoCorrect`, `secureTextEntry`,
/// `multiline`.
ImeOptions imeOptionsFor(
    const std::string &keyboardType,
    const std::string &returnKeyType,
    const std::string &autoCapitalize,
    bool autoCorrect,
    bool secure,
    bool multiline);

/// The canvas's Ecore IMF context, the documented way a native Tizen app
/// reaches the TV's on-screen keyboard: one per window, created on the first
/// edit, as Elementary's own entry and flutter-tizen do. One field edits
/// through it at a time. Main thread only.
class ImeSession final {
 public:
  explicit ImeSession(Evas *evas);
  ~ImeSession();

  ImeSession(const ImeSession &) = delete;
  ImeSession &operator=(const ImeSession &) = delete;

  /// Whether an input method answered: false in a simulator with none.
  bool isAvailable() const;
  /// Starts editing `client`: applies `options`, focuses the context and asks
  /// for the panel unless `options.showPanel` is false.
  void begin(ImeClient &client, const ImeOptions &options);
  /// Ends the edit: drops the composition, unfocuses the context, hides the panel.
  void end();
  /// Offers a key to the input method first, as flutter-tizen does: every key
  /// while the panel shows, every key but navigation and remote keys while
  /// editing. True when the input method took it.
  bool filter(const Ecore_Event_Key &key, bool isDown);
  void showPanel();
  void hidePanel();
  bool isPanelShown() const;
  /// Tells the input method where the caret is: `position` in characters,
  /// the rest in canvas pixels.
  void caretMoved(int position, Evas_Coord x, Evas_Coord y, Evas_Coord width, Evas_Coord height);
  /// Drops the composition, for text that changed under it.
  void reset();

 private:
  static void onCommit(void *self, Ecore_IMF_Context *context, void *info);
  static void onPreedit(void *self, Ecore_IMF_Context *context, void *info);
  static void onDeleteSurrounding(void *self, Ecore_IMF_Context *context, void *info);
  static Eina_Bool onSurrounding(void *self, Ecore_IMF_Context *context, char **text, int *cursor);
  static void onPanelState(void *self, Ecore_IMF_Context *context, int state);

  Ecore_IMF_Context *context_{nullptr};
  ImeClient *client_{nullptr};
  Ecore_IMF_Input_Panel_State panelState_{ECORE_IMF_INPUT_PANEL_STATE_HIDE};
};

} // namespace rntizen
