# TextInput

What React Native's `<TextInput>` needs from a host that runs Android's
JavaScript, and how react-native-tizen gives it: a field drawn with Evas, keys
from the remote or a keyboard, and the TV's on-screen keyboard through Ecore
IMF. The keyboard and IMF facts are in [tizen-input.md](tizen-input.md),
sections 3 and 10.

Tags as in [tizen-input.md](tizen-input.md): **D** documented by the source,
**I** inferred, **S** verified in the simulator, **H** verified on hardware.
Paths: `RN/` is react-native-tvos 0.88.0-0 in `node_modules`, `RA/` its
`ReactAndroid/src/main/java/com/facebook/react/`, `RC/` its
`ReactCommon/react/renderer/`, `host/` this package's `runtime/host/src/`.

## 1. What Android's TextInput does

| Fact | Tag | Source |
|------|-----|--------|
| `TextInput.js` loads `AndroidTextInput` and renders it only when `Platform.OS === 'android'`; on `tizen` it renders nothing | D | `RN/Libraries/Components/TextInput/TextInput.js:75`, `625` |
| `TextInputState.js` sends the `focus` and `blur` commands only on `ios` and `android` | D | `RN/Libraries/Components/TextInput/TextInputState.js:98` to `110`, `129` to `141` |
| Commands: `focus`, `blur`, `setTextAndSelection(mostRecentEventCount, text, start, end)`; `-1` is unset, an unset `end` is `start`, a `null` text leaves the text | D | `RN/Libraries/Components/TextInput/AndroidTextInputNativeComponent.js:644` to `646`; `RA/views/textinput/ReactTextInputManager.kt:163` to `190` |
| JS text or selection applies only when its event count is at least the native one (`canUpdateWithEventCount`); a JS text update sends no `onChange` | D | `RA/views/textinput/ReactEditText.kt:759` to `821` |
| A native edit increments the counter twice: once into the shadow tree's state, once into `topChange`, which is sent after the state | D | `RA/views/textinput/ReactTextInputTextWatcher.kt:50` to `67` |
| `topChange` `{text, eventCount, target, selection: {start, end}}`; `topSelectionChange` `{selection: {start, end}}`, sent only when it differs from the last one; `topSubmitEditing` `{target, text, action}` (`submit`, `next`, `previous`); `topEndEditing` `{target, text}`; `topFocus` and `topBlur` `{target}`; `topKeyPress` `{key}`; `topContentSizeChange` `{contentSize: {width, height}, target}` (padding included) | D | `RA/views/textinput/ReactTextChangedEvent.kt:25` to `40`, `ReactTextInputSelectionEvent.kt:22` to `34`, `ReactTextSelectionWatcher.kt:27` to `45`, `ReactTextInputSubmitEditingEvent.kt:21` to `36`, `ReactTextInputEndEditingEvent.kt:27` to `35`, `RA/uimanager/events/FocusEvent.kt:20` to `25`, `ReactTextInputKeyPressEvent.kt:17` to `27`, `ReactTextContentSizeWatcher.kt:26` to `50` |
| Focus sends `topFocus`; blur sends `topBlur`, then `topEndEditing` | D | `RA/views/textinput/ReactTextInputManager.kt:915` to `925` |
| An editor action submits when `submitBehavior` says so and blurs on `blurAndSubmit`; `getNonDefaultSubmitBehavior` is `newline` for multiline, `blurAndSubmit` otherwise | D | `RA/views/textinput/ReactTextInputManager.kt:928` to `976`; `RC/components/textinput/BaseTextInputProps.cpp:246` to `250` |
| A hardware Enter fires the editor action at key up | D | [aosp] `TextView.java:9923` to `9930` |
| `onKeyPress` comes from the soft keyboard: a commit of at most two UTF-16 units is that text, a delete is `Backspace`, a newline `Enter`; it is sent before the edit | D | `RA/views/textinput/ReactEditTextInputConnectionWrapper.kt:18` to `150` |
| A text watcher's change goes out before the selection's: span watchers run after text watchers | D | [aosp] `SpannableStringBuilder.java:577` to `581` |
| `maxLength` is a `LengthFilter`: it keeps what fits in UTF-16 units and never splits a surrogate pair | D | [aosp] `InputFilter.java:175` to `190` |
| A password shows U+2022 for each character | D | [aosp] `PasswordTransformationMethod.java:269` |
| The caret blinks every 500 ms and is 2 dp wide | D | [aosp] `Editor.java:186`; `text_cursor_material.xml:22` to `24` |
| An EditText centers its text vertically (`gravity` `center_vertical`) | D | [aosp] `styles.xml:560` |
| Android's descriptor pads an input with the theme's EditText padding read from Java when the input sets none | D | `RC/components/textinput/platform/android/react/renderer/components/androidtextinput/AndroidTextInputComponentDescriptor.h:37` to `130` |
| Android's shadow node measures through the TextLayoutManager's `measureCachedSpannableById` and `measureLines`; its state serializes to a `MapBuffer` | D | same directory, `AndroidTextInputShadowNode.cpp:42`, `96`, `AndroidTextInputState.cpp:19` to `37` |
| `Props::getDiffProps` and `getDiffPropsImplementationTarget`, which `AndroidTextInputProps.h` overrides, exist only under `RN_SERIALIZABLE_STATE`, which React Native defines for Android builds alone | D | `RC/core/Props.h:62` to `75`; `RN/ReactCommon/cmake-utils/react-native-flags.cmake:32` to `33` |
| The Android view config's `topChange`, `topFocus`, `topBlur`, `topSelectionChange` and `topContentSizeChange` come from the base Android config | D | `RN/Libraries/NativeComponent/BaseViewConfig.android.js:28`, `134`, `140`, `179`, `188` |

## 2. What the host does

| Piece | How | Where |
|-------|-----|-------|
| JavaScript | A Babel plugin reads `Platform.OS` as `'android'` in `TextInput.js` and `TextInputState.js` only, so both take Android's path; it runs before Expo inlines `Platform.OS` | `src/metro/tizen-android-branch.ts` |
| Props | React Native's `BaseTextInputProps`, plus the keyboard props Android parses (`secureTextEntry`, `keyboardType`, `returnKeyType`, `autoCorrect`, `selectTextOnFocus`, `caretHidden`, and `showSoftInputOnFocus`, unset unless given). `AndroidTextInputProps.h` cannot build without `RN_SERIALIZABLE_STATE`, and defining it for a few files would change `Props`' layout under the rest of React Native | `host/fabric/TextInputProps.*` |
| Layout and state | `AndroidTextInputShadowNode`'s rules: a single line measures at infinite width, the placeholder when empty, the state's text while the tree's has not changed, an older event count never overwrites. Measured by the host's text measurer. No theme padding | `host/fabric/TextInputShadowNode.*`, `TextInputComponentDescriptor.cpp` |
| Drawing | One textblock built like a `<Text>`'s (`textblockContent`), the selection as its `backing`, the composition underlined, a password as U+2022, the placeholder in `placeholderTextColor` or the text color at half opacity, centered vertically unless `textAlignVertical` says otherwise, clipped to the content box, scrolled to keep the caret in it. A 2-point caret blinks at 500 ms and hides while text is selected | `host/views/TextInputLayer.*` |
| Events and commands | Android's names, payloads, order and double event count, sent straight through the view's event emitter; `focus`, `blur`, `setTextAndSelection` with Android's event count rule; `autoFocus` on mount | `host/views/TextInputView.cpp` |
| Keys | While a field has the keyboard, every key goes to it before the remote handler: to the input method first, then to the field. A key either uses, down or up, never becomes a remote event | `host/input/KeyboardFocus.*`, `RemotePresses.cpp`, `host/views/TextInputKeys.cpp` |
| On-screen keyboard | One Ecore IMF context per window, made on the first focus as EFL's entry makes it; focus shows the panel unless `showSoftInputOnFocus` is false; the commit, preedit, delete-surrounding and surrounding callbacks edit the text; the caret's position and place are reported on every change | `host/input/ImeSession.cpp`, `ImeOptions.cpp` |
| Native alert, error screen | They keep the keys: the field sees none while one shows | `host/ReactHost.cpp` |
| UTF-8 | A value from JavaScript or the input method is decoded as the WHATWG Encoding Standard's UTF-8 decoder decodes, in its `replacement` mode: one U+FFFD for each maximal invalid subpart, so a truncated sequence never swallows the characters after it; overlong forms, surrogates and anything past U+10FFFF are invalid. Those constraints are Unicode's "Best Practices for Using U+FFFD" ([whatwg-enc], sections 4.1 and 8.1.1). `textTransform` decodes the same way | `host/text/Utf8.cpp` |

Keys while a field has the keyboard:

| Key | Does |
|-----|------|
| A key with text and no Ctrl, Alt or Win (letters, digits, space) | types it; `onKeyPress` with the text |
| `BackSpace`, `Delete` | deletes before or after the caret, or the selection |
| `Left`, `Right` (Shift extends) | moves the caret; at the first or last character, the field blurs and the key goes on as a remote key, as focus leaves an EditText on Android TV |
| `Up`, `Down` | moves a line in a multiline field; otherwise blurs and goes on |
| `Home`, `End` | start, end |
| `Return`, `KP_Enter` | a newline when `submitBehavior` is `newline`; otherwise, at key up, submits, or reopens the panel if the user closed it (the web IME's "OK opens the IME", [sam-ime]) |
| `Select` (the IME's Done) | submits; in a `newline` field, closes the panel |
| `Cancel` (the IME closed itself) | nothing; the field keeps the keyboard |
| `XF86Back`, `Escape` | at key up, closes the panel if it shows, else blurs; never reaches `BackHandler` |
| anything else (media keys, Tab) | goes on to the remote handler unchanged |

## 3. Verified in the simulator

`example/src/screens/text-input` (`--entry src/screens/text-input/main.ts`),
keys from `rn-tizen simulator key`, Ubuntu's EFL 1.28.1 and the `xim` context.

| Fact | Tag |
|------|-----|
| `autoFocus` focuses on mount: `focus`, and the `xim` context is made | S |
| Typing sends `keyPress`, then `change` with event counts 2 apart, then `selectionChange`; the caret draws and blinks | S |
| Left moves the caret; a key typed mid-text inserts there; Backspace deletes before the caret | S |
| Return on a single-line field: `submitEditing`, `blur`, `endEditing`, and no `select` remote event | S |
| A `submitBehavior: 'submit'` field keeps the keyboard after Return | S |
| `secureTextEntry` draws one dot per character; `maxLength` 8 keeps 8 and still sends `keyPress` for the rest, as Android does | S |
| Back blurs a field whose panel is closed; the app stays open | S |
| Multiline: Return types a newline, Up and Down move between lines | S |
| A field JavaScript controls (`value={text.toUpperCase()}`) shows the uppercased text after each key | S |
| `editable={false}` refuses `focus` | S |
| Shift and the arrows select; the selection draws behind the text and hides the caret | S |
| Down on a single-line field blurs it and reaches `useTVEventHandler` as `down` | S |

## 4. Gaps

| Where | Problem | Tag |
|-------|---------|-----|
| TV | The on-screen keyboard path (panel, commit, preedit, `Select`, `Cancel`) is untested on hardware | I |
| `TextInputView` | Nested `<Text>` children draw in the field's own style, not their own | D |
| `TextInputView` | No `onScroll`, no `selection` handles, no clipboard, no `contextMenuHidden` | D |
| `TextInputKeys` | Hardware keys send `onKeyPress` as the soft keyboard does; Android sends none for them | D |
| `TextInputShadowNode` | No baseline: `alignItems: 'baseline'` ignores the field (Android's uses `measureLines`) | D |
| `ImeSession` | A key's device name cannot be read (tizen-input.md, section 3): any `Select` key acts as the IME's Done | I |
| Simulator | `xim` has no input method server, so the IMF callbacks (commit, preedit) never run there | S |

## 5. Checks for a TV

On the TV, from `example/`: `rn-tizen run --entry src/screens/text-input/main.ts`.

1. The search field has the keyboard at start: the Samsung keyboard appears,
   with Search as its action key.
2. Typing on it fills the field and lists `change` events; a composing IME
   (Korean, or word prediction) underlines its composition.
3. Done submits and closes the keyboard (`submit`, `blur`, `endEditing`).
4. OK on the password field opens a password layout; Back closes the keyboard
   first (`Cancel`), a second Back blurs, a third leaves the app.
5. After Back closed the keyboard, OK opens it again.
6. The keyboard's own Backspace deletes (as a key or a delete-surrounding).
7. The app's log names the IMF context (`Input method context: ...`) and no
   warnings follow.

## Sources

| ID | Source |
|----|--------|
| aosp | Android at `android-15.0.0_r1`, `frameworks/base`: `core/java/android/widget/TextView.java`, `widget/Editor.java`, `text/SpannableStringBuilder.java`, `text/InputFilter.java`, `text/method/PasswordTransformationMethod.java`, `core/res/res/values/styles.xml`, `core/res/res/drawable/text_cursor_material.xml`: https://android.googlesource.com/platform/frameworks/base/+/refs/tags/android-15.0.0_r1 |
| sam-ime | Samsung, Keyboard/IME (web): https://developer.samsung.com/smarttv/develop/guides/user-interaction/keyboardime.html |
| whatwg-enc | WHATWG, Encoding Standard, Living Standard updated 19 October 2026: the UTF-8 decoder, https://encoding.spec.whatwg.org/#utf-8-decoder, and an error in `replacement` mode, https://encoding.spec.whatwg.org/#concept-encoding-process |
