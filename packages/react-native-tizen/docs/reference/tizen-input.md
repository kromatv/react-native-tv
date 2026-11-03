# Tizen TV input

How a Samsung remote, a keyboard and the on-screen keyboard reach a native
Tizen app, and what react-native-tvos code expects to receive. Read it before
touching `runtime/host/src/input/`.

Every fact carries a tag and a source ID from [Sources](#sources):

| Tag | Meaning |
|-----|---------|
| **D** | documented by the source |
| **I** | inferred, from what is said next to it |
| **H** | verified on hardware: 2024 Samsung The Frame, Tizen 9.0, its Samsung Smart Remote |
| **S** | verified in simulator: Ubuntu's EFL 1.28 on X11 (Xvfb, `xdotool`), not a TV |

Local headers are cited as `<file>:<line>` under
`~/tizen-studio/platforms/tizen-9.0/tizen/rootstraps/tizen-9.0-device.core/usr/include/`.

## 1. The path of a key

| Step | Fact | Tag | Source |
|------|------|-----|--------|
| Compositor | Key grabs are requests to the Tizen keyrouter (`tizen_keyrouter_set_keygrab` on the window's `wl_surface`) | D | [efl-tz] 3431 |
| Client | `ecore_wl2` turns each `wl_keyboard.key` into `ECORE_EVENT_KEY_DOWN` or `ECORE_EVENT_KEY_UP` carrying an `Ecore_Event_Key` | D | [efl-up] 614 to 651, `ecore-input-1/Ecore_Input.h:51` |
| Default | "each key event is delivered to the focused window by default"; a grab is for "special hardware key events" | D | [tz-grab] |
| On a TV | Arrows and `Return` arrive with no grab; `XF86Back` arrives only once grabbed | H | |
| Ungrabbed media key | Play/Pause without a grab shows a system "Not Available" popup; grabbing `XF86PlayBack` removes it | D | [ft-319] |

## 2. Remote keys

`key` is the `Ecore_Event_Key::key` value (section 6). Web name and code are the
TVInputDevice values for the same button ([sam-rc] table 3), useful when porting
web TV code. Basic is the classic remote with digits and color keys; Smart is the
Samsung Smart Remote, "only supported on Samsung UHD TVs", whose missing keys sit
behind its "123" on-screen pad ([sam-rc]).

| Button | Remote | `key` | Web name, code | Grab | Tag, source |
|--------|--------|-------|----------------|------|-------------|
| Arrows | both | `Up` `Down` `Left` `Right` | ArrowUp 38, ArrowDown 40, ArrowLeft 37, ArrowRight 39 | no | H; D [sam-net] |
| OK | both | `Return` | Enter 13 | no | H; D [sam-net] |
| Back (RETURN) | both | `XF86Back` | Back 10009 | yes | H; D [sam-net] |
| Back, long press | Smart | `XF86Exit` | Exit 10182 | never | D [sam-net], [sam-rc] |
| EXIT | basic | `XF86Exit` | Exit 10182 | never | D [sam-net] |
| Play/Pause | Smart | `XF86PlayBack` | MediaPlayPause 10252 | yes | H; D [sam-net] |
| Play | basic | `XF86AudioPlay` | MediaPlay 415 | yes | D [sam-net] (its table prints `XF86AudioPlayv`, its code sample `XF86AudioPlay`) |
| Pause, Stop, Record | basic | `XF86AudioPause` `XF86AudioStop` `XF86AudioRecord` | MediaPause 19, MediaStop 413, MediaRecord 416 | yes | D [sam-net] |
| Rewind | basic | `XF86AudioRewind`, long press `XF86PreviousChapter` | MediaRewind 412, long press MediaTrackPrevious 10232 | yes | D [sam-net], [sam-rc] |
| Fast forward | basic | `XF86AudioNext`, long press `XF86NextChapter` | MediaFastForward 417, long press MediaTrackNext 10233 | yes | D [sam-net]; Samsung's own engine maps `XF86AudioNext` to 417 [starfish] |
| CH+, CH- | both | `XF86RaiseChannel` `XF86LowerChannel` | ChannelUp 427, ChannelDown 428 | arrive without | H; D [sam-net] |
| CH (pressed) | Smart | `XF86ChannelGuide` | Guide 458 | untested | D [sam-net]; code from [starfish] |
| CH LIST, PRE-CH | basic | `XF86ChannelList` `XF86PreviousChannel` | ChannelList 10073, PreviousChannel 10190 | untested | D [sam-net], [starfish] |
| VOL+, VOL-, Mute (VOL pressed on Smart) | both | `XF86AudioRaiseVolume` `XF86AudioLowerVolume` `XF86AudioMute` | VolumeUp 447, VolumeDown 448, VolumeMute 449 | untested | H: never reach the app ungrabbed. D: the web guide registers them [sam-rc]; Tizen lists them grabbable on TV [tz-grab] |
| 0 to 9, minus | basic | `0` to `9`; minus is `Minus` in [sam-net] and `-` in [tz-grab] | 48 to 57, Minus 189 | untested | D; minus unverified |
| A B C D | basic | `XF86Red` `XF86Green` `XF86Yellow` `XF86Blue` | ColorF0Red 403 to ColorF3Blue 406 | yes | D [sam-net] |
| INFO | basic | `XF86Info` | Info 457 | yes | D [sam-net] |
| TOOLS | basic | `XF86SimpleMenu` | Tools 10135 | untested | D [sam-net]; code from [starfish] |
| MENU (settings) | both | `XF86SysMenu` | Menu 18 [sam-rc]; [starfish] emits 10133 | never | I: flutter-tizen drops it [ft-view] 285 |
| Home (Smart Hub) | both | `XF86Home` | none public; 10071 in [starfish] | never | I: name from [starfish], [tizenfx-elm] |
| SOURCE | basic | `XF86Display` ("toggle the video source") | Source 10072 | never | I: name from [tz-grab] |
| Power | both | `XF86PowerOff` | none | never | D name [tz-grab] |
| Mic | Smart | `XF86BTVoice` | none public; 10224 in [starfish] | arrives without | H |
| 123, color pad | Smart | `XF86More`, `XF86Color` | none public; 10148, 10385 in [starfish] | unknown | I: delivery to apps never documented |
| E-MANUAL, SEARCH, CAPTION, P.SIZE, EXTRA, SLEEP | basic | `XF86EManual` `XF86Search` `XF86Caption` `XF86PictureSize` `XF86ExtraApp` `XF86Sleep` | 10146, 10225, 10221, 10140, 10253, 10150 | untested | D codes [sam-rc]; names [starfish] |
| SUBTITLE, 3D | basic | `XF86Subtitle` `XF863D` | 3D 10199 | untested | D [tz-grab], [sam-rc] |
| App buttons (Netflix, Prime Video...) | Smart | unknown | none | unknown | not documented |

## 3. Keyboards and the on-screen keyboard as key sources

| Fact | Tag | Source |
|------|-----|--------|
| A USB or Bluetooth keyboard sends ordinary XKB keysyms (`Escape`, `BackSpace`, `KP_Enter`, letters with `string` set) | I | [efl-up] 614 to 640 |
| Web apps see a keyboard's ESC as Back, F5 Smart Hub, F6 Source, F7 ChannelList, F8 VolumeMute, F9 VolumeDown, F10 VolumeUp, F11 ChannelDown, F12 ChannelUp | D | [sam-ime] table 5 |
| The TV reports remote volume keys as F10, F9, F8 to web pages; remote keys "are mapped to arbitrary scan codes (such as F1~F12 and Left Shift) by the TV OS" | D | [starfish-key] 75 to 84; [ft-319] |
| On-screen keyboard keys arrive as Ecore key events from a device named `ime` | D | [ft-124] |
| Its Done arrives as `key == "Select"`, its cancel as `Cancel`; flutter-tizen treats `Select` from the IME as the input action on TV | D | [ft-text] 351 to 356, [starfish-efl] 1443 to 1446, [sam-net] |
| `ecore_device_name_get`, `ecore_device_class_get` and `ecore_device_subclass_get` are commented out of the 9.0 header, so public API cannot read a key's device (`ime`): `Select` and `Cancel` can only be matched by name | D | `ecore-1/Ecore_Common.h:3373`, `3452`, `3475` |

## 4. Grabs

| Mode | `Ecore_Wl2_Window_Keygrab_Mode` | `Elm_Win_Keygrab_Mode` | Meaning | Third-party app |
|------|------|------|---------|---------|
| Shared | 1 | 256 | gets the key "together with the other client windows" | no |
| Topmost | 2 | 512 | gets the key "only when window is top of the stack" | yes, without the keygrab privilege |
| Override exclusive | 3 | 2048 | exclusive whatever the stacking; another client's grab overrides it | no |
| Exclusive | 4 | 1024 | exclusive whatever the stacking | no |

Sources: `ecore-wl2-1/Ecore_Wayland2.h:138` to `151` ("third-party application can
use ECORE_WL2_WINDOW_KEYGRAB_TOPMOST mode", the others "Not for third-party
application"), `605` to `634`; `elementary-1/elm_win_legacy.h:145` to `166`. **D**

| API | Fact | Tag | Source |
|-----|------|-----|--------|
| `eext_win_keygrab_set(win, name)`, `eext_win_keygrab_unset` | Public API; "Key callback is only called when the window located in topmost", so topmost mode | D | `efl-extension/efl_extension.h:83` to `115` |
| | `Eina_Bool eext_win_keygrab_set(Elm_Win *obj, const char *key)` and `_unset` alike, since 2.4. The Tizen 10.0 reference titles the module "Efl Extension (Deprecated)" | D | [eext-keyevent] |
| `ecore_wl2_window_keygrab_set(win, name, 0, 0, 0, mode)`, `_unset`, `_list_set` | `@platform`, privilege `http://tizen.org/privilege/keygrab`, topmost allowed without it; fails with `ALREADY_IN_PROGRESS` or `PERMISSION_DENIED` | D | `ecore-wl2-1/Ecore_Wayland2.h:605` to `716` |
| | flutter-tizen grabs its key list with this call and `TOPMOST` | D | [ft-window] 803 to 808 |
| `elm_win_keygrab_set` | Commented out of the 9.0 header, so not callable | D | `elementary-1/elm_win_legacy.h:1285` |
| ElmSharp `Window.KeyGrabEx` (eext), `WinKeyGrab(name, KeyGrabMode)`, `EvasObject.KeyGrab(name, exclusive)` | All `[Obsolete]` since API 12; `KeyGrab` is an Evas grab inside the app's own canvas | D | [tizenfx-elm] |
| NUI `Window.GrabKeyTopmost`, `GrabKey(key, mode)`, `UngrabKey...` | "If this function is called between key down and up events of a grabbed key, an application doesn't receive the key up event" | D | [tizenfx-nui] 1106 to 1168 |
| Privilege | The Tizen 9.0 SDK's .NET privilege list (57 entries) has no `keygrab`, so a store app gets topmost only | I | `platforms/tizen-9.0/common/privileges/privilege-dotnet.properties` |
| Cost | `eext_win_keygrab_set`/`_unset` call `elm_win_keygrab_set`/`_unset`, which call `ecore_wl2_window_keygrab_set`/`_unset`: each sends one keyrouter request per keycode, then a display sync, and blocks until the compositor answers. 23 keys are 23 round trips | D | `libefl-extension.so` imports (rootstrap), [efl-tz-win] 10515 to 10538, [efl-tz-pin] 3468 to 3531 |
| Release | The keyrouter registers a destroy listener on the grabbing surface (or client) and drops its grabs when that surface (or client) goes, so a window being destroyed needs no ungrab | D | [e-kr] 563 to 590, 652 to 681; [e-kr-mod] 196 to 206 |
| TV grab list | Tizen's guide lists as grabbable on TV: volume, mute, power, `XF86Menu`, `XF86Home`, `XF86Back`, all `XF86Audio*` media keys, digits, `-`, channel keys, `XF86SysMenu`, `XF86SimpleMenu`, arrows, `Return`, `XF86Close`, `XF86Info`, color keys, `XF86Subtitle`, `XF863D` and more | D | [tz-grab] |

The Tizen guide calls `XF86Close` "Exit key on the remote control"; Samsung's TV
pages and engine say `XF86Exit` ([sam-net], [starfish-efl] 811). Trust Samsung.

## 5. Keys the system keeps

| Key | Behaviour | Tag | Source |
|-----|-----------|-----|--------|
| Volume, mute | Never reach an ungrabbed app; a grab was not tried | H | |
| Back, long press (`XF86Exit`) | "Long-pressing the Return/Exit key forces the application to terminate." "Do not register the Return/Exit key long-press." | D | [sam-exit] |
| Home, Power, Source, settings Menu | Not documented for apps | I | |
| Web mandatory keys | Arrows, Enter, Back are always delivered and "An application cannot register" them | D | [sam-tvid] |

## 6. `keyname`, `key`, `keycode`

| Field | Content | Tag | Source |
|-------|---------|-----|--------|
| `keyname` | Name of the keysym the key has with no modifiers (xkb "maskless" state); `Keycode-<n>` when unnamed | D | [efl-up] 592 to 599, 1072; [efl-tz] 1862 |
| `key` | "The key symbol": the keysym in the current state, modifiers and compose applied ("shift+1 == exclamation") | D | `ecore-input-1/Ecore_Input.h:254`, `evas-1/Evas_Legacy.h:342` |
| `string`, `compose` | UTF-8 text the key produces, NULL for non-text keys | D | [efl-up] 633 to 640 |
| `keycode` | XKB keycode, evdev code + 8 ("xkb rules reflect X broken keycodes, so offset by 8"); TV remotes use the TV's own layout | D | [efl-up] 1065; [ft-wiki] |
| `timestamp` | Milliseconds from `wl_keyboard.key`, undefined base | D | [wayland] |
| `event_flags` | `ECORE_EVENT_FLAG_REPEAT` on Tizen's synthetic repeats; `ECORE_EVENT_FLAG_CANCEL` after a `Cancel` keysym | D | `ecore-1/Ecore_Common.h:3482` to `3490`; [efl-tz] 992 to 996, 1867 to 1873 |
| `dev` | `Ecore_Device`: name (`ime`), class, subclass `ECORE_DEVICE_SUBCLASS_REMOCON` | D | `ecore-1/Ecore_Common.h:3299`; [ft-124] |
| `window` | Window the event targets | D | `ecore-input-1/Ecore_Input.h:257` |

Why flutter-tizen moved from `keyname` to `key`: engine PR #234 (February 2022)
replaced `key->keyname` with `event->key` (renamed `symbol`) under "Use consistent
words" and keyed its new TV table `kSymbolToScanCode` by `event->key` ([ft-234]).
**D** for the change; the PR gives no reason. **I**: `key` is what EFL documents as
"the key symbol", what its own key-grab sample compares ([tz-grab]), what Samsung's
starfish reads ([starfish-efl] 1241), and what the IME's `Select` and `Cancel` are
matched on. For remote keys the two fields are equal, since a remote sets no
modifiers (**I**); they differ for keyboards (`KP_Up` versus `KP_8` under NumLock).
Tizen's repeat timer also builds a repeat's `keyname` from the modified symbol
([efl-tz] 1899). Never key on `keycode`: remote codes are arbitrary ([ft-319]).

## 7. Key up, repeat, long press

| Fact | Tag | Source |
|------|-----|--------|
| Every release produces `ECORE_EVENT_KEY_UP` with the same struct | D | `ecore-input-1/Ecore_Input.h:52`; [efl-up] 646 to 651 |
| Tizen's `ecore_wl2` repeats a held key itself: a timer re-sends `KEY_DOWN` with `ECORE_EVENT_FLAG_REPEAT`, only for keys the keymap marks repeating, with separate rates for Left/Right and Up/Down; each repeat's timestamp is advanced by the rate | D | [efl-tz] 1717 to 1757, 1880 to 1925 |
| Rates: `ecore_wl2_input_keyboard_repeat_get`, `_horizontal_way_repeat_get`, `_vertical_way_repeat_get` | D | `ecore-wl2-1/Ecore_Wl2.h:2161` to `2214` |
| "Some Tizen devices do not repeat arrow keys while they are held down"; flutter-tizen synthesizes repeats for keyboards only, never for `ECORE_DEVICE_SUBCLASS_REMOCON`, and treats a second `KEY_DOWN` of the same keycode before its `KEY_UP` as a device repeat | D | [ft-199]; [ft-window] 594 to 617 |
| So a remote repeat may arrive without the flag; detect it as "already down" | I | same |
| Whether the Frame's remote repeats, and how fast | unknown | |
| Samsung turns some long presses into other keys: Back to `XF86Exit`, Rewind to `XF86PreviousChapter`, Fast forward to `XF86NextChapter` | D | [sam-net], [sam-rc] |
| Holding Back about 2 s delivers `XF86Exit` to the app, never `XF86Back`, and the system then closes the app: no `hardwareBackPress` reaches JavaScript | H | `rn-tizen run`, debug log `Unmapped key: XF86Exit` |
| Holding OK gives `longSelect` twice (start, release); holding Right or Left gives `longRight`/`longLeft` twice; Play/Pause gives `playPause`, Channel up `channelUp` | H | `rn-tizen run`, the example's `useTVEventHandler` log |
| Other long presses must be timed: `KEY_UP.timestamp` minus the first `KEY_DOWN.timestamp`, or a timer from the first `KEY_DOWN` | I | |
| On X11, a held key repeats as further `KEY_DOWN`s with the same keycode and no `KEY_UP` between them (detectable auto-repeat), the first after about 660 ms, then every 45 ms; `key->window` and `key->event_window` equal `ecore_evas_window_get()` of the app's window | S | `rn-tizen simulator`, `xdotool keydown` |

## 8. Pointer, touch, swipe

| Fact | Tag | Source |
|------|-----|--------|
| No Samsung page read documents a Smart Remote pointer, touch pad or swipe for apps | D (absence) | [sam-rc], [sam-net], [sam-mouse], [sam-touch] |
| A USB or Bluetooth mouse is supported; web apps default to `pointing-device-support` enabled on 2015 to 2020 models, disabled from 2021 | D | [sam-mouse] |
| A native app gets `ECORE_EVENT_MOUSE_*`; flutter-tizen also sets the TV's pointer support and the `vd-cursors` theme | D | [ft-window] 31 to 37, 310 to 327, 399 to 500 |
| Touch exists only on the LPF5DS model (IR touch) | D | [sam-touch] |

## 9. Voice

| Fact | Tag | Source |
|------|-----|--------|
| The Smart Remote mic button reaches the app as `XF86BTVoice`, ungrabbed | H | |
| Whether the TV also opens its assistant on that press, or on a long press | unknown | |
| Samsung's documented voice API is JavaScript only: `webapis.voiceinteraction` (`setCallback`, `listen`; `onnavigation`, `onselection`, `onplay`, `onpause`, `onrewind`, `onfastforward`; `ontitleselection` from 6.5 receives "the string input via voice"); privilege `http://developer.samsung.com/privilege/voicecontrol`, `required_version` 6.0; no `onsearch` | D | [sam-voice] |
| No voice API is documented for .NET or native TV apps | D (absence) | [sam-net], [sam-voice] |
| `webapis.microphone` (`getMicrophones`, `enableDevice`) needs `http://developer.samsung.com/privilege/microphone`; that privilege is in the SDK's TV web list | D | [sam-mic]; `platforms/tizen-9.0/tv-samsung/privileges/privilege-wrt.properties:8` |
| Tizen's native `voice_control.h` and `stt.h` ship in the 9.0 rootstrap, privilege `http://tizen.org/privilege/recorder`; whether a store app may use them on a Samsung TV is not documented | D, then unknown | `voice_control.h:88`, `stt.h:386` |

## 10. Text input and the on-screen keyboard

| Fact | Tag | Source |
|------|-----|--------|
| .NET: focusing an `Entry` shows the IME; to close it on Done or Cancel, handle the `Select` and `Cancel` key events and move focus | D | [sam-net] |
| Web: OK on a focused input opens the IME; Done, Go, Search, Back, or Up at its top row close it; from Tizen 6.0 Up and Back are delivered as Cancel and the IME has no Cancel key | D | [sam-ime] tables 2 to 4 |
| Web codes: Done 65376, Cancel 65385, other IME keys 229; layout ABC by default, QWERTY for email and password; floating style from Tizen 6.5 | D | [sam-ime] |
| Native API, Ecore IMF: `ecore_imf_init` (865), `ecore_imf_context_default_id_get` (929), `ecore_imf_context_add` (988), `client_window_set` (1019), `client_canvas_set` (1046), `focus_in` (1165), `focus_out` (1187), `filter_event` (1352), `input_panel_show` (1710), `input_panel_hide` (1719), `input_panel_layout_set` (1730), `input_panel_return_key_type_set` (1844), `input_panel_state_get` (1919), `input_panel_event_callback_add` with `ECORE_IMF_INPUT_PANEL_STATE_EVENT` (1933); callbacks `ECORE_IMF_CALLBACK_COMMIT`, `PREEDIT_CHANGED`, `DELETE_SURROUNDING` (231 to 233) | D | `ecore-imf-1/Ecore_IMF.h` |
| `ecore_imf_context_show` is deprecated since 5.5 for `input_panel_show` | D | `ecore-imf-1/Ecore_IMF.h:1062` |
| flutter-tizen's recipe: one context per window, `client_window_set` with the window id; on focus `focus_in` then `input_panel_show`; while the panel is shown, every key goes to the IMF first, and while editing every key except arrows, `Return`, `KP_Enter`, `Select` and `XF86*` | D | [ft-imc] 120 to 175, 225 to 243, 254 to 272 |
| `elm_win_window_id_get` gives the `Ecore_Window` an Elementary window needs for `client_window_set` | I | `elementary-1/elm_win_legacy.h:1196` |
| EFL's own entry takes the default context id (falling back to the `evas` canvas type's), then `client_window_set(ecore_evas_window_get(ecore_evas_ecore_evas_get(evas)))`, `client_canvas_set(evas)`, a retrieve-surrounding callback and the `COMMIT`, `DELETE_SURROUNDING` and `PREEDIT_CHANGED` callbacks | D | [efl-entry] 3069 to 3103 |
| On focus out it calls `ecore_imf_context_reset` then `focus_out`; on focus in, `focus_in` then the cursor position | D | [efl-entry] 141, 160, 187, 222 |
| A delete-surrounding event's `offset` and `n_chars` count characters from the cursor; the surrounding text it hands back is plain UTF-8 with the cursor in characters, masked with `*` when the hint has `ECORE_IMF_INPUT_HINT_SENSITIVE_DATA` | D | [efl-entry] 4712 to 4790, 5059 to 5085; `Ecore_IMF.h:579` |
| `cursor_position_set` takes "New cursor position in characters" and should follow every confirmed cursor move | D | `ecore-imf-1/Ecore_IMF.h:1222` to `1232` |
| `input_panel_enabled_set(true)`: "the input panel will be shown when the widget is clicked or has focus" | D | `ecore-imf-1/Ecore_IMF.h:1786` to `1795` |
| Layouts (`NORMAL`, `EMAIL`, `URL`, `PHONENUMBER`, `NUMBERONLY` with signed and decimal variations, `PASSWORD`...), return keys (`DEFAULT`, `DONE`, `GO`, `JOIN`, `NEXT`, `SEARCH`, `SEND`...), autocapital types, input hints (`AUTO_COMPLETE`, `SENSITIVE_DATA`, `MULTILINE`) and `ECORE_IMF_INPUT_MODE_INVISIBLE` for passwords | D | `ecore-imf-1/Ecore_IMF.h:308`, `340` to `344`, `355` to `370`, `394` to `403`, `412` to `427`; [efl-entry] 3104 to 3107 |
| A key handed to `ecore_imf_context_filter_event` is an `Ecore_IMF_Event_Key_Down` or `_Up` built from the `Ecore_Event_Key`; flutter-tizen copies its fields one by one | D | `ecore-imf-1/Ecore_IMF.h:729` to `742`; [ft-imc] 89 to 118 |
| flutter-tizen ignores an empty preedit (the "pre-edit end stage") and lets the commit that follows replace the composition | D | [ft-text] 75 to 85, 101 to 111 |
| Ubuntu's EFL 1.28.1 ships the `scim`, `wayland` and `xim` context modules; in the simulator's Xvfb the default context is `xim`, no input method server runs, and a key's `string` carries what the desktop keyboard typed | S | `rn-tizen simulator`, `Input method context: xim` in the log |
| An empty textblock lays out one line of its default style (43 px at 28 px font) and a cursor at 0 has that line's height; it measures in the style's default font, not a run's | S | a probe against the simulator's EFL 1.28.1 |

## 11. What react-native-tvos expects

Read from react-native-tvos 0.88.0-0, the version this repository pins ([rntvos]).

| | Android TV | tvOS |
|---|------------|------|
| Channel | `onHWKeyEvent` `{eventType, eventKeyAction, tag}` on the device emitter | `onHWKeyEvent` on `NativeTVNavigationEventEmitter`, same keys |
| Short press | one event at key up, `eventKeyAction: 1`; down events (`0`) only with `ReactFeatureFlags.enableKeyDownEvents` (java 188 to 193) | tap recognizer, one event with `1` (state Ended, mm 69 to 79) |
| Long press | select and arrows: after 300 ms of repeats, `long<Type>` `0` once, then `long<Type>` `1` at release (java 110, 145 to 182) | long press recognizer: `long<Type>` `0` at Began, `1` at Ended |
| Back | the Activity emits `hardwareBackPress`; `BackHandler.android.js` listens to it | `menu` event; `BackHandler.ios.js:79` treats `menu` as Back |
| Event types | `select playPause play pause next previous rewind fastForward record stop up down left right info captions menu 0 to 9 channelUp channelDown delete bookmark avrInput avrPower dvr guide red green blue yellow stbInput stbPower tv tvInput window teletext`; long: `longSelect longUp longDown longLeft longRight longPlayPause longRewind longFastForward longChannelUp longChannelDown` (java 31 to 99) | `menu playPause select longPlayPause longSelect longUp longDown longLeft longRight up down left right pageUp pageDown swipeUp swipeDown swipeLeft swipeRight pan focus blur` (`RCTTVRemoteHandlerConstants.mm:8` to `32`) |
| `eventKeyAction` | `0` ACTION_DOWN, `1` ACTION_UP | `-1` unknown, `0` began, `1` ended; swipes and pan carry none |

All **D** from the files named. Two consequences, **I**:

- Android's default stream and tvOS's are the same shape: a short press is one
  event with `1`, a hold is `long<Type>` `0` then `1`. Code written for both
  platforms works if Tizen emits that shape.
- Tizen resolves React Native's JavaScript as Android ([with-tizen] 73 to 96), so
  Back goes through `hardwareBackPress`, but `Platform.OS` is `'tizen'`
  ([platform-tizen] 20): app code that pairs down and up events only when
  `Platform.OS === 'android'` takes its tvOS path on Tizen.

Android's `onBackPressed` runs from `Activity.onKeyUp` ([android-keyup]), so Back
fires on release there. **D**

## 12. Implications for react-native-tizen

### Key table

Match on `key`. Anything absent passes through unhandled.

| `key` | Emits |
|-------|-------|
| `Up` `Down` `Left` `Right` | `up` `down` `left` `right` |
| `Return` `KP_Enter` | `select` |
| `XF86Back` `Escape` | Back: `onRequestClose` of the last `Modal` mounted, else `hardwareBackPress` |
| `XF86PlayBack` `XF86AudioPlayPause` | `playPause` |
| `XF86AudioPlay` `XF86AudioPause` `XF86AudioStop` `XF86AudioRecord` | `play` `pause` `stop` `record` |
| `XF86AudioRewind` | `rewind` |
| `XF86AudioNext` (Samsung's fast forward button) `XF86AudioForward` (keyboards) | `fastForward` |
| `XF86NextChapter` `XF86PreviousChapter` `XF86AudioPrev` | `next` `previous` `previous` |
| `XF86RaiseChannel` `XF86LowerChannel` | `channelUp` `channelDown` |
| `XF86ChannelGuide` | `guide` |
| `XF86Info` | `info` |
| `XF86SimpleMenu` `XF86Menu` | `menu` (Android's meaning; on tvOS `menu` is Back) |
| `XF86Caption` `XF86Subtitle` | `captions` |
| `XF86Red` `XF86Green` `XF86Yellow` `XF86Blue` | `red` `green` `yellow` `blue` |
| `0` to `9` | `0` to `9` |
| `BackSpace` | `delete` |
| `XF86BTVoice` | `voice` (Tizen only: no react-native-tvos platform emits it) |
| Left unhandled on purpose | `XF86Exit`, `XF86Home`, `XF86SysMenu`, volume, mute, `XF86PowerOff` |

### Grab list

Topmost, with `eext_win_keygrab_set`, once the window exists: `XF86Back`
`XF86PlayBack` `XF86AudioPlayPause` `XF86AudioPlay` `XF86AudioPause`
`XF86AudioStop` `XF86AudioRecord` `XF86AudioRewind` `XF86AudioNext`
`XF86AudioForward` `XF86AudioPrev` `XF86NextChapter` `XF86PreviousChapter`
`XF86RaiseChannel` `XF86LowerChannel` `XF86ChannelGuide` `XF86Info`
`XF86SimpleMenu` `XF86Caption` `XF86Red` `XF86Green` `XF86Yellow` `XF86Blue`.

Never grab `XF86Exit` ([sam-exit]; flutter-tizen does, against that rule),
`XF86Home`, `XF86SysMenu`, volume, power or `XF86BTVoice` (it arrives anyway, and
a grab might take it from the assistant: unverified). Grabbing mapped keys follows
flutter-tizen's "handled by the app first but not by the system" list
([ft-view] 19 to 31). Check each new grab on a TV.

### Event contract

1. Track pressed keycodes. A `KEY_DOWN` with `ECORE_EVENT_FLAG_REPEAT`, or for a
   keycode already down, is a repeat: never a new press.
2. First `KEY_DOWN` of `select` or an arrow: start a 300 ms timer. If it fires
   while held, emit `long<Type>` with `0`.
3. `KEY_UP`: emit `long<Type>` with `1` if the timer fired, else `<type>` with `1`.
   Back emits one `hardwareBackPress` here and nowhere else, or, while a
   `Modal` is mounted, one `onRequestClose` to the last one mounted, as a
   Modal's dialog consumes Back on Android ([rntvos] modal 291 to 321).
4. Offer a switch matching `enableKeyDownEvents` that also emits `<type>` with `0`
   on the first `KEY_DOWN`. Android TV and tvOS move focus natively on the press
   itself; a JavaScript focus engine fed only by step 3 reacts at release.
5. While a text field has the keyboard, give the key to
   `ecore_imf_context_filter_event` first (every key while the panel shows,
   every key but navigation and `XF86*` otherwise, as in [ft-imc] 254 to 262),
   then to the field; a key either uses, down and up, is no remote event.
   `Select` (the IME's Done) or `Return` submits; Back closes the panel, then
   blurs; `Cancel` and a panel `HIDE` leave the field focused. The full key
   table is in [text-input.md](text-input.md), section 2.
6. Add the focused view's `tag` to the payload once a native focus model exists,
   as Android does.

### What the host does

`runtime/host/src/input/RemoteInput.cpp` follows the event contract above,
steps 1 to 3; the key table and grab list are `RemoteKeyMap.cpp`. A long press
is offered for `select`, the arrows and `playPause` only, the keys tvOS reports
holds for: a held Rewind or Fast forward becomes a chapter key on Samsung's
remote already. A key whose release never comes (a long press the TV turned
into another key, a pause, a focus loss) is forgotten after a second of silence,
on `focus,out`, and on pause, so it cannot stay "down" for good. It never
ungrabs: the grabs go with the window's surface (section 4). The 23 ungrabs it
made at exit were 23 compositor round trips, the likely cause of shutdown
growing from 55 ms to about 670 ms on the TV when they arrived (I: in the
simulator the shim makes grabs free and shutdown takes 2 to 5 ms either way).

While a `<TextInput>` has the keyboard, `ReactHost` hands every key to
`KeyboardFocus` before `RemoteInput` maps it (step 5), unless the native alert or
the error screen shows: `KeyboardFocus` offers it to the window's Ecore IMF
context (`ImeSession`), then to the field, and remembers which of the two took
each keycode so its release goes there too. A key the field passes on (an
arrow at its edge, a media key) is a remote key as usual. See
[text-input.md](text-input.md).

Verified in simulator (**S**), with the shapes `useTVEventHandler` received:

| Input | Events |
|-------|--------|
| Tap Right | `right` 1 |
| Hold Right 1.2 s (about 15 repeats) | `longRight` 0 at 300 ms, `longRight` 1 at release |
| Tap, then hold Return 0.8 s | `select` 1; `longSelect` 0, `longSelect` 1 |
| Hold `XF86Back` 1.5 s | one `hardwareBackPress`, at release |
| `XF86AudioNext`, `7`, hold `XF86AudioRewind` 1 s | `fastForward` 1, `7` 1, one `rewind` 1 |
| `XF86Exit` | nothing; logged as unmapped and passed on |

### Gaps

| Where | Problem | Tag |
|-------|---------|-----|
| `RemoteInput.cpp` | Step 4 (key down events, `enableKeyDownEvents`) and step 6 (`tag`) are not done | D |
| `ImeSession.cpp` | Step 5 runs in the simulator against `xim` with no input method server; the TV's panel, commits, preedit, `Select` and `Cancel` are unverified | S, I |
| `RemoteInput.cpp` | Grabbing is unverified for the chapter keys, `XF86AudioRecord`, `XF86ChannelGuide`, `XF86SimpleMenu`, `XF86Caption`; a failed grab now logs a warning | I |
| `RemoteKeyMap.cpp` | `voice` is not a react-native-tvos event type; it is a Tizen addition | D [rntvos] |
| `RemoteKeyMap.cpp` | `XF86Menu` appears on no Samsung TV remote table; it stays mapped for keyboards. `menu` also means Back on tvOS | I |
| `simulator/guest/key.sh` | The chapter keys `XF86NextChapter`/`XF86PreviousChapter` have no X keysym, so the simulator cannot send `next` | S |
| Input as a whole | No mouse events, no voice API | D |

Open questions for the TV: does a held remote key repeat, at what rate and with
which flag; does a grab of volume keys succeed; what the 123 and app buttons
send; whether `XF86BTVoice` also wakes the assistant; the minus key's name.

## 13. The network remote

What `rn-tizen key` drives (`src/remote/`): the TV's remote API, a WebSocket
on the LAN. Samsung does not document it for third parties; the facts below
come from a client library that ships against it, and from the TV.

| Fact | Tag | Source |
|------|-----|--------|
| The remote is a WebSocket at `wss://<tv>:8002/api/v2/channels/samsung.remote.control?name=<name>&token=<token>`; the name is base64, the token is sent only once the TV issued one. Port 8001 is the same API without TLS | D | [samtvws] `connection.py` `_format_websocket_url`, `_is_ssl_connection`; `helper.py` `serialize_string` |
| The client turns certificate verification off for 8002 (`sslopt = {"cert_reqs": ssl.CERT_NONE}`) | D | [samtvws] `connection.py` `open()` |
| Node's built-in WebSocket fails the TLS handshake with the TV; the `ws` package with `rejectUnauthorized: false` is accepted. Under Bun, `ws` is accepted given `tls: { rejectUnauthorized: false }`: Bun's `ws` ignores the top-level option, as it does against a local self-signed server | H, 2026-11-03, Node 26.5, Bun 1.4 | `src/remote/tv-socket.ts`, a handshake with each |
| The TV answers a connection it accepts with `ms.channel.connect`, the token it issued in `data.token`; one the viewer refused with `ms.channel.unauthorized`; a failure with `ms.error` and `data.message` | D | [samtvws] `event.py`; `connection.py` `open()`, `_check_for_token`, `_websocket_event` |
| `ms.channel.timeOut` is in no source here; `rn-tizen` treats it as an unanswered request | I | `src/remote/remote-session.ts` |
| A key press is `{"method":"ms.remote.control","params":{"Cmd":"Click","DataOfCmd":"KEY_ENTER","Option":"false","TypeOfRemote":"SendRemoteKey"}}`; `Press` and `Release` hold a key | D | [samtvws] `remote.py` `send_key`, `command.py` |
| Without a token the TV asks its viewer to allow the client by name. With the token it issued, it accepts with nobody at the TV and issues no new token | H, 2026-11-03 | `rn-tizen key`, then a handshake with the saved token |

What it means here: `rn-tizen key` opens the socket through `ws`, giving both
spellings of the TLS option so one code path runs under Node and Bun, bounds a
message at 64 KiB, and keeps the token mode 0600 in the state directory.

## Sources

| ID | Source |
|----|--------|
| sam-net | Samsung, Tizen .NET TV, User Interaction: https://developer.samsung.com/smarttv/develop/tizen-net-tv/guides/user-interaction.html |
| sam-rc | Samsung, Remote Control (web): https://developer.samsung.com/smarttv/develop/guides/user-interaction/remote-control.html |
| sam-tvid | Samsung, TVInputDevice API: https://developer.samsung.com/smarttv/develop/api-references/tizen-web-device-api-references/tvinputdevice-api.html |
| sam-ime | Samsung, Keyboard/IME (web): https://developer.samsung.com/smarttv/develop/guides/user-interaction/keyboardime.html |
| sam-exit | Samsung, Terminating Applications: https://developer.samsung.com/smarttv/develop/guides/fundamentals/terminating-applications.html |
| sam-mouse | Samsung, Mouse: https://developer.samsung.com/smarttv/develop/guides/user-interaction/mouse.html |
| sam-touch | Samsung, Touch: https://developer.samsung.com/smarttv/develop/guides/user-interaction/touch.html |
| sam-voice | Samsung, VoiceInteraction API: https://developer.samsung.com/smarttv/develop/api-references/samsung-product-api-references/voiceinteraction-api.html |
| sam-mic | Samsung, Microphone API: https://developer.samsung.com/smarttv/develop/api-references/samsung-product-api-references/microphone-api.html |
| tz-grab | Tizen, Grabbing Hardware Key Events, removed from docs.tizen.org in July 2025: https://github.com/Samsung/tizen-docs/blob/50fab02c3dfca185e9edc91057b4c0f14a9eedc6/docs/application/native/guides/ui/efl/key-grab.md |
| eext-keyevent | Tizen native API reference 10.0, Efl Extension Hardware Keyevent, read 2026-11-03: https://samsungtizenos.com/docs/application/native/api/10.0.0/common/group__EFL__EXTENSION__HARDWARE__KEYEVENT__GROUP |
| efl-tz | Tizen's EFL, `ecore_wl2_input.c`: https://git.tizen.org/cgit/platform/upstream/efl/tree/src/lib/ecore_wl2/ecore_wl2_input.c?h=tizen |
| efl-tz-pin | the same file at `515314b`, read 2026-11-03: https://git.tizen.org/cgit/platform/upstream/efl/tree/src/lib/ecore_wl2/ecore_wl2_input.c?id=515314b5cf255032969b8a0be447084550fd4302 |
| efl-tz-win | Tizen's EFL, `src/lib/elementary/efl_ui_win.c` at `197c848`: https://git.tizen.org/cgit/platform/upstream/efl/tree/src/lib/elementary/efl_ui_win.c?id=197c84873c3fd25ae6e81e05e6712e28bb007856 |
| e-kr | Tizen's compositor, enlightenment `src/bin/server/e_keyrouter_wl.c` at `d8dc46a`: https://git.tizen.org/cgit/platform/upstream/enlightenment/tree/src/bin/server/e_keyrouter_wl.c?id=d8dc46a55e52f68e404a68f5c4bcf87aadb06dd1 |
| e-kr-mod | Tizen's keyrouter module, `src/e_mod_keyrouter_list.c` at `f621bb4`: https://git.tizen.org/cgit/platform/core/uifw/e-mod-tizen-keyrouter/tree/src/e_mod_keyrouter_list.c?id=f621bb493bbe1f1364638e40ce244766cc1d9e7e |
| efl-up | Upstream EFL, same file: https://github.com/Enlightenment/efl/blob/ffb6f58fa93bcce65648345efe4cdbb07006db7a/src/lib/ecore_wl2/ecore_wl2_input.c |
| ft-view, ft-window, ft-text, ft-imc | flutter-tizen embedder at `44c61a1`, `flutter/shell/platform/tizen/`: `flutter_tizen_view.cc`, `tizen_window_ecore_wl2.cc`, `channels/text_input_channel.cc`, `tizen_input_method_context.cc`: https://github.com/flutter-tizen/embedder/tree/44c61a1f069a11a166fbeef0cae17855e152ab40/flutter/shell/platform/tizen |
| efl-entry | Upstream EFL v1.25.1, `src/lib/edje/edje_entry.c`, Elementary's entry: https://github.com/Enlightenment/efl/blob/v1.25.1/src/lib/edje/edje_entry.c |
| ft-234 | flutter-tizen engine PR #234: https://github.com/flutter-tizen/engine/pull/234 |
| ft-124 | flutter-tizen engine PR #124: https://github.com/flutter-tizen/engine/pull/124 |
| ft-199 | flutter-tizen embedder PR #199: https://github.com/flutter-tizen/embedder/pull/199 |
| ft-319 | flutter-tizen issue #319 and its comments: https://github.com/flutter-tizen/flutter-tizen/issues/319 |
| ft-wiki | flutter-tizen wiki, Key mapping: https://github.com/flutter-tizen/flutter-tizen/wiki/Key-mapping |
| starfish-key, starfish-efl | Samsung starfish at `675a841`: `src/core/dom/KeyboardEvent.cpp`, `src/public/bridge/efl/LWEWebViewEFL.cpp`: https://github.com/Samsung/starfish/tree/675a84147893cb68b771bb6957eb02090b40e4fc |
| starfish | both starfish files |
| tizenfx-elm | TizenFX at `792e1a2`, `src/ElmSharp/ElmSharp/Window.cs`, `EvasObject.cs`, `EvasKeyEventArgs.cs`: https://github.com/Samsung/TizenFX/tree/792e1a26b8f18e65716707b170965e631ef07d9a/src/ElmSharp/ElmSharp |
| tizenfx-nui | TizenFX at `792e1a2`, `src/Tizen.NUI/src/public/Window/Window.cs` |
| rntvos | react-native-tvos 0.88.0-0 in `node_modules/react-native`: `ReactAndroid/src/main/java/com/facebook/react/modules/core/ReactAndroidHWInputDeviceHelper.java` (java), `React/Base/RCTTVNavigationEventNotification.mm` (mm), `React/Base/RCTTVRemoteHandler.m`, `Libraries/Utilities/BackHandler.{android,ios}.js`, `ReactAndroid/src/main/java/com/facebook/react/views/modal/ReactModalHostView.kt` (modal) |
| with-tizen, platform-tizen | this package: `src/metro/with-tizen.ts`, `react-native-overrides/Libraries/Utilities/Platform.tizen.ts` |
| wayland | Wayland protocol, `wl_keyboard.key`: https://wayland.freedesktop.org/docs/html/apa.html#protocol-spec-wl_keyboard |
| android-keyup | Android, `Activity.onKeyUp`: https://developer.android.com/reference/android/app/Activity#onKeyUp(int,%20android.view.KeyEvent) |
| samtvws | samsung-tv-ws-api, `samsungtvws/connection.py`, `remote.py`, `event.py`, `helper.py`, `command.py` on `master`, read 2026-11-03: https://github.com/xchwarze/samsung-tv-ws-api/tree/master/samsungtvws |
