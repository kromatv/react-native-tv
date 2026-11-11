# TextInput

What React Native's `<TextInput>` needs from a host that runs Android's
JavaScript, and how react-native-webos gives it: a field drawn with SkParagraph,
keys from the remote or a keyboard, and the TV's on-screen keyboard through SDL's
text input. The keyboard and SDL facts are in
[webos-window-input.md](webos-window-input.md), sections 10 and 15.

Tags: **D** documented by the source, **I** inferred, **S** verified in the
simulator, **H** verified on hardware. No row here is **H** yet; what a webOS 25 TV showed
of a field, its events and LG's keyboard is in
[hardware-verification.md](hardware-verification.md). Paths: `RN/` is react-native-tvos 0.88.0-0 in `node_modules`, `RA/` its
`ReactAndroid/src/main/java/com/facebook/react/`, `RC/` its
`ReactCommon/react/renderer/`, `host/` this package's `runtime/host/src/`.

## 1. What Android's TextInput does

| Fact | Tag | Source |
|------|-----|--------|
| `TextInput.js` loads `AndroidTextInput` and renders it only when `Platform.OS === 'android'`; on `webos` it renders nothing | D | `RN/Libraries/Components/TextInput/TextInput.js:75`, `625` |
| `TextInputState.js` sends the `focus` and `blur` commands only on `ios` and `android` | D | `RN/Libraries/Components/TextInput/TextInputState.js:98` to `110`, `129` to `141` |
| Commands: `focus`, `blur`, `setTextAndSelection(mostRecentEventCount, text, start, end)`; `-1` is unset, an unset `end` is `start`, a `null` text leaves the text | D | `RN/Libraries/Components/TextInput/AndroidTextInputNativeComponent.js:644` to `646`; `RA/views/textinput/ReactTextInputManager.kt:163` to `190` |
| JS text or selection applies only when its event count is at least the native one (`canUpdateWithEventCount`); a JS text update sends no `onChange` | D | `RA/views/textinput/ReactEditText.kt:759` to `821` |
| A text from JavaScript goes in with `Editable.replace(0, length, text)`, which keeps the selection "at the same relative position": an end strictly inside the old text moves to `offset * newLength / oldLength`, one at the old end to the new end, one at 0 stays; an empty text is `setText(null)` | D | `RA/views/textinput/ReactEditText.kt:790` to `812`; [aosp] `SpannableStringBuilder.java`, `replace` (the `adjustSelection` block) and `updatedIntervalBound` |
| A native edit increments the counter twice: once into the shadow tree's state, once into `topChange`, which is sent after the state | D | `RA/views/textinput/ReactTextInputTextWatcher.kt:50` to `67` |
| `topChange` `{text, eventCount, target, selection: {start, end}}`; `topSelectionChange` `{selection: {start, end}}`, sent only when it differs from the last one; `topSubmitEditing` `{target, text, action}` (`submit`, `next`, `previous`); `topEndEditing` `{target, text}`; `topFocus` and `topBlur` `{target}`; `topKeyPress` `{key}`; `topContentSizeChange` `{contentSize: {width, height}, target}` (padding included) | D | `RA/views/textinput/ReactTextChangedEvent.kt:25` to `40`, `ReactTextInputSelectionEvent.kt:22` to `34`, `ReactTextSelectionWatcher.kt:27` to `45`, `ReactTextInputSubmitEditingEvent.kt:21` to `36`, `ReactTextInputEndEditingEvent.kt:27` to `35`, `RA/uimanager/events/FocusEvent.kt:20` to `25`, `ReactTextInputKeyPressEvent.kt:17` to `27`, `ReactTextContentSizeWatcher.kt:26` to `50` |
| Focus sends `topFocus`; blur sends `topBlur`, then `topEndEditing` | D | `RA/views/textinput/ReactTextInputManager.kt:915` to `925` |
| An editor action submits when `submitBehavior` says so and blurs on `blurAndSubmit`; `getNonDefaultSubmitBehavior` is `newline` for multiline, `blurAndSubmit` otherwise | D | `RA/views/textinput/ReactTextInputManager.kt:928` to `976`; `RC/components/textinput/BaseTextInputProps.cpp:246` to `250` |
| A hardware Enter fires the editor action at key up | D | [aosp] `TextView.java:9923` to `9930` |
| `onKeyPress` comes from the soft keyboard: a commit of at most two UTF-16 units is that text, a delete is `Backspace`, a newline `Enter`; it is sent before the edit | D | `RA/views/textinput/ReactEditTextInputConnectionWrapper.kt:18` to `150` |
| `keyboardType`: `numeric` is a number with a sign and a decimal point, `number-pad` a plain number, `decimal-pad` a number with a decimal point, `phone-pad` a phone, `email-address`, `url` and `visible-password` their text variations, anything else text; `secureTextEntry` on a number makes a numeric password. `returnKeyType` is Done unless `go`, `next`, `none`, `previous`, `search` or `send`; `autoCapitalize` is sentences unless `none`, `characters` or `words` | D | `RA/views/textinput/ReactTextInputManager.kt:697` to `709`, `715` to `768`, `1138` to `1142`, `1192` to `1203`; `RA/views/textinput/ReactEditText.kt:1064` to `1074` |
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
| JavaScript | A Babel plugin reads `Platform.OS` as `'android'` in `TextInput.js` and `TextInputState.js` only, so both take Android's path; it runs before Expo inlines `Platform.OS` | `src/metro/webos-android-branch.ts` |
| Props | React Native's `BaseTextInputProps`, plus the keyboard props Android parses (`secureTextEntry`, `keyboardType`, `returnKeyType`, `autoCorrect`, `selectTextOnFocus`, `caretHidden`, and `showSoftInputOnFocus`, unset unless given). `AndroidTextInputProps.h` cannot build without `RN_SERIALIZABLE_STATE`, and defining it for a few files would change `Props`' layout under the rest of React Native | `host/fabric/TextInputProps.*` |
| Layout and state | `AndroidTextInputShadowNode`'s rules: a single line measures at infinite width, the placeholder when empty, the state's text while the tree's has not changed, an older event count never overwrites. Measured by the host's text measurer, SkParagraph. No theme padding | `host/fabric/TextInputShadowNode.*`, `TextInputComponentDescriptor.cpp` |
| Drawing | One SkParagraph laid out like a `<Text>`'s: a single line at its own width or the box's, whichever is wider, so `textAlign` works in the box and a long line scrolls; several lines at the box's width. The selection is the paragraph's `getRectsForRange` boxes painted behind the text, the composition is underlined, a password is U+2022 for each character, the placeholder is in `placeholderTextColor` or the text color at half opacity. Centered vertically unless `textAlignVertical` says otherwise, clipped to the content box, scrolled to keep the caret in it | `host/views/TextInputLayer.*`, `TextInputContent.cpp` |
| Caret | A 2-point bar at the caret's line (`getLineMetrics`), after the boxes of the line's text before it. It blinks at 500 ms on a main loop timer that runs only while the field has the keyboard and nothing is selected, and each blink redraws the scene. Up and Down land on `getGlyphPositionAtCoordinate` of the next line at the caret's x | `host/views/TextInputLayer.cpp` |
| Events and commands | Android's names, payloads, order and double event count, sent straight through the view's event emitter; `focus`, `blur`, `setTextAndSelection` with Android's event count rule; `autoFocus` on mount. A text from JavaScript moves the caret as `Editable.replace` does (section 1) | `host/views/TextInputView.cpp`, `host/input/EditBuffer.*` |
| Keys | While a field has the keyboard, every key goes to it before the remote handler. A key the field uses, down or up, never becomes a remote event | `host/input/KeyboardFocus.*`, `RemotePresses.cpp`, `host/views/TextInputKeys.cpp` |
| On-screen keyboard | SDL's text input on the app's window: focus starts it with the field's keyboard props, which raises the panel; committed text and compositions arrive as SDL's text events. What SDL cannot say (`url`, `phone-pad`, a `returnKeyType`, `showSoftInputOnFocus={false}`) is logged once and has no effect | `host/input/ImeSession.cpp`, `ImeOptions.cpp`; [webos-window-input.md](webos-window-input.md), section 15 |
| Error screen | It keeps the keys and the text events: the field sees none while it shows. The native alert will do the same once it is ported | `host/ReactHost.cpp`, `ReactHostWindow.cpp` |
| Pause, reload, focus loss | The keys the field holds are forgotten, as the remote's are: their releases may never come. The field keeps the keyboard through a pause, as an EditText does | `host/ReactHost.cpp`, `ReactHostWindow.cpp` |
| UTF-8 | A value from JavaScript or the keyboard is decoded as the WHATWG Encoding Standard's UTF-8 decoder decodes, in its `replacement` mode: one U+FFFD for each maximal invalid subpart, so a truncated sequence never swallows the characters after it; overlong forms, surrogates and anything past U+10FFFF are invalid. Those constraints are Unicode's "Best Practices for Using U+FFFD" ([whatwg-enc], sections 4.1 and 8.1.1). `textTransform` decodes the same way | `host/text/Utf8.cpp` |

Keys while a field has the keyboard, by SDL scancode:

| Key | Does |
|-----|------|
| Text from SDL (`SDL_EVENT_TEXT_INPUT`: the panel, or a keyboard's key) | types it; `onKeyPress` with the text when it is at most two UTF-16 units |
| A key whose keycode is a printable character, without Ctrl, Alt or the GUI key | nothing by itself: its text comes as the event above. It is the field's, so a digit is never also a remote key. After Back lowered the panel it goes on to the remote handler |
| `BACKSPACE`, `DELETE` | deletes before or after the caret, or the selection. The panel's own deletions arrive as these keys |
| `CLEAR` (the panel's Clear all) | empties the field |
| `LEFT`, `RIGHT` (Shift extends) | moves the caret; at the first or last character, the field blurs and the key goes on as a remote key, as focus leaves an EditText on Android TV |
| `UP`, `DOWN` | moves a line in a multiline field; otherwise, and past the first or last line, blurs and goes on |
| `HOME`, `END` | start, end |
| OK: `RETURN`, `KP_ENTER`, the remote's raw 352 and 353 | raises the panel again, at key up, when the user closed it and no key has typed since the field took the keyboard; otherwise a newline when `submitBehavior` is `newline`, else submits at key up |
| Back: `WEBOS_BACK`, `AC_BACK`, `ESCAPE` | at key up, lowers the panel if it is up, else blurs; never reaches `BackHandler` |
| anything else (media keys, colors, Tab) | goes on to the remote handler unchanged |

## 3. Verified in the simulator

`example/src/screens/text-input` (`src/screens/text-input/main.ts`), 2026-11-11,
stock SDL 3 on X11 under Xvfb, Mesa llvmpipe. X11 has no on-screen keyboard and
the simulator's keys are SDL events the host makes itself, so the keyboard is
played through the app's control FIFO (`platform/SimulatorControl.h`,
`simulator/guest/control.sh`): `text <text>` commits, `compose <text>` composes,
`keyboard shown|hidden` is the panel; `key Shift+Left` is a key with Shift.

| Fact | Tag |
|------|-----|
| `autoFocus` focuses on mount: `focus`, and the caret draws at the placeholder's start | S |
| Committed text sends `keyPress` (two UTF-16 units at most), then `change` with event counts 2 apart, then `selectionChange`; the caret draws after it and after a trailing space | S |
| Left moves the caret; text committed mid-text inserts there; Backspace deletes before the caret | S |
| OK on a single-line field: `submitEditing`, `blur`, `endEditing`, and no `select` remote event | S |
| A `submitBehavior: 'submit'` field keeps the keyboard after OK; `textAlign: 'center'` centers the text and the caret follows it | S |
| `secureTextEntry` draws one dot per character; `maxLength` 8 keeps 8 and still sends `keyPress` for the rest, as Android does | S |
| A line longer than the field scrolls sideways so the caret stays in the box, and stays scrolled once blurred | S |
| Back blurs a field whose panel is down; the app stays open | S |
| Multiline: OK types a newline, Up and Down move between lines to the nearest character, Up on the first line blurs and reaches `useTVEventHandler` as `up` | S |
| A field JavaScript controls (`value={text.toUpperCase()}`) shows the uppercased text after each commit, and a commit mid-text leaves the caret after it | S |
| `editable={false}` refuses `focus` | S |
| Shift and Left select; the selection draws behind the text and hides the caret; a commit replaces it | S |
| Down on a single-line field blurs it and reaches `useTVEventHandler` as `down` | S |
| A composition draws underlined and sends `change` for each step; its commit is one `change` | S |
| With the panel played as shown: Back lowers it and the field keeps the keyboard, text then does not arrive, OK starts text input again without submitting, a second OK submits | S |

## 4. Gaps

| Where | Problem | Tag |
|-------|---------|-----|
| TV | Nothing was run on a TV: the panel, its text, compositions, its Enter, Clear all and Back are read from SDL's source only | I |
| `ImeOptions` | `keyboardType` `url` and `phone-pad`, every `returnKeyType` but `done`, and `showSoftInputOnFocus={false}` have no effect; the three number keyboards are one | D |
| `ImeSession` | No surrounding text reaches the keyboard: SDL has no call for it, so prediction sees only what it typed itself | D |
| `TextInputKeys` | After Back lowered the panel, a hardware keyboard types nothing until OK starts text input again: SDL sends text only while text input is started | D |
| `TextInputKeys` | A hardware keyboard's first Enter after the panel went down by itself raises the panel instead of submitting, unless a key typed first | I |
| `TextInputView` | Nested `<Text>` children draw in the field's own style, not their own | D |
| `TextInputView` | No `onScroll`, no `selection` handles, no clipboard, no `contextMenuHidden` | D |
| `TextInputView` | A `textTransform` that changes a text's length puts the caret and the selection off by the difference: they are placed by the value's offsets | I |
| `TextInputKeys` | Hardware keys send `onKeyPress` as the soft keyboard does; Android sends none for them | D |
| `TextInputLayer` | The caret keeps no side at a soft line break: at the break it draws at the start of the next line | D |
| `TextInputShadowNode` | No baseline: `alignItems: 'baseline'` ignores the field (Android's uses `measureLines`) | D |
| Simulator | Stock SDL on X11 has no on-screen keyboard: the panel, its events and the text model are played by the control FIFO, not by a compositor | S |

## 5. Checks for a TV

On the TV, from `example/`: `rn-webos run --entry src/screens/text-input/main.ts`.

1. The search field has the keyboard at start: LG's keyboard appears about a
   second after launch, once the window has keyboard focus.
2. Typing on it fills the field and lists `change` events; a composing keyboard
   (Korean, or word prediction) underlines its composition, and its commit is
   one `change`.
3. The panel's Enter submits and closes the keyboard (`submit`, `blur`,
   `endEditing`).
4. OK on the password field opens a password layout. Back closes the keyboard
   first, a second Back blurs, a third leaves the app. Note whether the first
   Back reaches the app (the panel then closes by a stop) or the panel takes it
   (the app then sees `SDL_EVENT_SCREEN_KEYBOARD_HIDDEN`).
5. After Back closed the keyboard, OK opens it again, and Back then closes it
   first again: this is where SDL's stale shown flag decides.
6. The panel's Backspace deletes, and its Clear all empties the field.
7. The remote's digit keys while a field has the keyboard: do they type?
8. A USB keyboard: the panel goes down at the first key, the keys type, Enter
   submits, Back blurs.
9. The app's log has no `Cannot start text input` line.

## Sources

| ID | Source |
|----|--------|
| aosp | Android at `android-15.0.0_r1`, `frameworks/base`: `core/java/android/widget/TextView.java`, `widget/Editor.java`, `text/SpannableStringBuilder.java`, `text/InputFilter.java`, `text/method/PasswordTransformationMethod.java`, `core/res/res/values/styles.xml`, `core/res/res/drawable/text_cursor_material.xml`: https://android.googlesource.com/platform/frameworks/base/+/refs/tags/android-15.0.0_r1 (`SpannableStringBuilder.java` read 2026-11-11 from the Gitiles page, which shows no line numbers) |
| whatwg-enc | WHATWG, Encoding Standard, Living Standard updated 19 October 2026: the UTF-8 decoder, https://encoding.spec.whatwg.org/#utf-8-decoder, and an error in `replacement` mode, https://encoding.spec.whatwg.org/#concept-encoding-process |
