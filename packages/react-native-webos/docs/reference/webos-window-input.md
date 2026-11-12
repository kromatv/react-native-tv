# webOS window and input

How a native app gets a fullscreen surface on an LG webOS TV, draws into it with
OpenGL ES, and receives the remote, the Magic Remote pointer and the on-screen
keyboard. Read it before touching the window, the event loop or
`runtime/host/src/input/`.

Every fact carries a tag and a source ID from [Sources](#14-sources):

| Tag | Meaning |
|-----|---------|
| **D** | documented: stated by a page, a guide or a comment of LG, webosbrew or the project named |
| **R** | read from source: the code or the firmware data does this, cited by file and line |
| **I** | inferred, from what is said next to it |

Nothing here was run on a TV by this project. A row that repeats what another
project saw on its own TV is tagged R and names the project. Release numbers are
webOS TV platform versions: 7.x is sold as webOS 22, 8.x as 23, 9.x as 24, 10.x
as 25, 11.x as 26 ([brew-lib] **D**).

## 1. The windowing system

| Fact | Tag | Source |
|------|-----|--------|
| The compositor is `surface-manager` (luna-surfacemanager, LSM), built on Qt's Wayland compositor. LG publishes the webOS OSE version; the TV build is closed | R, I | [lsm]; [jf] display.md 43 |
| The OSE compositor creates two shells: `wl_shell` (`QWaylandWlShell`) and `wl_webos_shell` (`WebOSShell`). Its source has no xdg-shell | R | [lsm] weboscorecompositor.cpp 241, 379 |
| Globals one project listed on its TV (SoC `o22n2`, Mali-G52, kernel 5.4.268): `wl_compositor` v3, `wl_shm` v1, `wl_seat` v4 three times, `wl_output` v2, `wl_shell` v1, `wl_webos_shell` v1, `wl_webos_foreign` v1, `wl_webos_input_manager` v1, `wl_starfish_pointer` v1, `text_model_factory` v1, `input_method` v2, `zwp_linux_dmabuf_v1` v3. No `xdg_wm_base` | R | [jf] display.md 45 to 62 |
| So an unpatched SDL or GLFW Wayland backend, which needs `xdg_wm_base`, shows nothing. "Upstream SDL2 does not run on a TV" | D | [jf] display.md 58 to 60; [brew-gamepad] |
| webOS advertises one `wl_seat` per input device. A client that binds only the first hears one remote | D | [brew-wl]; [jf] input.md 64 to 67; [sdl-rel] 2.30.0-webos.2 |
| webOS 1 offers `wl_compositor` v1 only. The TV's libwayland lacks `wl_proxy_get_version`, so a client cannot ask a proxy its version | D | [sdl-rel] 3.4.16-webos.5; [sdl2] SDL_waylandvideo.c 609 to 612 |

## 2. What a surface must carry

The sequence SDL-webOS, Kodi and RetroArch all follow ([sdl2] SDL_waylandwindow.c
2083 to 2099, [kodi] ShellSurfaceWebOSShell.cpp 36 to 113, [retroarch] 624 to 649, **R**):

1. `wl_compositor.create_surface`.
2. `wl_shell.get_shell_surface`, `set_class`, `set_title`, `set_toplevel`.
3. `wl_webos_shell.get_shell_surface(surface)`, add its listener.
4. `wl_webos_shell_surface.set_property(name, value)` for each property below.
5. `wl_webos_shell_surface.set_state(3)`, fullscreen.
6. `wl_egl_window_create`, `eglCreateWindowSurface`, draw, commit.

| Property | Value | Effect | Tag | Source |
|----------|-------|--------|-----|--------|
| `appId` | the app id, from the `APPID` environment variable | Ties the surface to the app. SDL refuses to create the window without `APPID`. LSM "only composites surfaces it can attribute to an app"; the id need not belong to an installed app | R | [sdl2] SDL_waylandwebos.c 139 to 144; [kodi] 86 to 100; [jf] display.md 71 to 78 |
| `displayAffinity` | display id, from `DISPLAY_ID`, else `"0"` | Which display shows it | R | [kodi] 92 to 101; [retroarch] 629 to 637; [lsm] webossurfaceitem.cpp 910 |
| `title` | window title | Shown by the system | R | [sdl2] SDL_waylandwindow.c 2246; [wam] 215 |
| `_WEBOS_WINDOW_TYPE` | `_WEBOS_WINDOW_TYPE_CARD` for an app | LSM assumes `CARD` when unset. SDL, Kodi and RetroArch never set it | R | [lsm] webossurfaceitem.cpp 76, webossurfacemodel.cpp 161 to 163 |
| `_WEBOS_ACCESS_POLICY_KEYS_BACK` | `"true"` or `"false"` | Back goes to the app instead of the system | R | [sdl2] 145 to 147; [kodi] 103 to 104; [wam] 221 to 223 |
| `_WEBOS_ACCESS_POLICY_KEYS_EXIT` | same | Exit goes to the app | R | [sdl2] 148 to 150; [wam] 224 |
| `_WEBOS_ACCESS_POLICY_KEYS_HOME`, `_GUIDE`, `_META` | same | Home, Guide, and from webOS 8 the keyboard super key | R, D | [sdl2] 151 to 159, SDL_hints.h 2726 to 2737; [kodi] 44 |
| `_WEBOS_ACCESS_POLICY_RIBBON` | `"false"` | Keeps the home launcher ribbon off the app | D | [sdl2] 160 to 162, SDL_hints.h 2739 to 2741 |
| `_WEBOS_ACCESS_POLICY_FORCESTRETCH` | `"true"` | Set by SDL on every window. Meaning not documented | R | [sdl2] 179 |
| `_WEBOS_WINDOW_CLASS` | `"1"` | Set by SDL only on request | R | [sdl2] 174 to 178 |
| `restore_cursor_position`, `cursor_fps`, `cloudgame_active` | see section 5 | Pointer and gamepad behaviour | R | [sdl2] 163 to 173 |
| `_WEBOS_LAUNCH_PREV_APP_AFTER_CLOSING`, `_WEBOS_LAUNCH_LAST_INPUT_APP_AFTER_CLOSING` | boolean | Parsed by LSM. No native app read here sets them | R | [lsm] webossurfaceitem.cpp 906 to 909 |
| `_WEBOS_LAUNCH_INFO_RECENT`, `_WEBOS_LAUNCH_INFO_REASON` | `"true"` | Found only in LG's Chromium 53 browser frame, not in LSM or any native app | R | GitHub code search, webosose/chromium53 |

`wl_webos_shell_surface.set_key_mask(bits)` is the newer way to claim keys: the
default mask `0xFFFFFFF8` leaves out home (1), back (2) and exit (4), and the
protocol says it "will eventually replace the string property based key
filtering" ([wl-ext] webos-shell.xml 153 to 201, **D**). LG's web app manager sets
both the property and the mask ([wam] 221 to 228, **R**); SDL, Kodi and RetroArch set
the property alone, so the property is enough on TVs (**I**).

Events of `wl_webos_shell_surface` ([wl-ext] webos-shell.xml 77 to 151, **D**) and
what SDL makes of them ([sdl2] SDL_waylandwebos.c 183 to 232, **R**):

| Event | Meaning | SDL |
|-------|---------|-----|
| `state_changed(state)` | 0 default, 1 minimized, 2 maximized, 3 fullscreen | minimized: `SDL_WINDOWEVENT_MINIMIZED`, then will and did enter background. Fullscreen after minimized: `RESTORED`, will and did enter foreground |
| `state_about_to_change(state)` | prepare only | minimized: will enter background |
| `close` | "the compositor requests closing of a window ... without explicitly quitting or killing the client" | flushes the queue, sends `SDL_WINDOWEVENT_CLOSE` |
| `exposed(rectangles)` | visible areas as `x,y,w,h,...,-1` | ignored. Kodi takes its window size from the first rectangle ([kodi] 46 to 49) |
| `position_changed(x, y)` | windowed surfaces only | `SDL_WINDOWEVENT_MOVED` |

## 3. Launch environment and lifecycle

| Fact | Tag | Source |
|------|-----|--------|
| A native app reads its id from `APPID` and its display from `DISPLAY_ID` | R | [sdl2] SDL_waylandwebos.c 139; [kodi] 86 to 96; [retroarch] 70, 629 |
| OSE's app manager passes the launch parameters as one JSON argument and sets `APP_ID`, `DISPLAY_ID`, `INSTANCE_ID`, `LAUNCHPOINT_ID`, `LS2_NAME`. The TV's app manager is closed; moonlight-tv parses `argv[1]` as JSON on TVs | R | [sam] NativeContainer.cpp 155 to 200; [moonlight] app_webos.c 39 to 48 |
| One project reports that the TV starts an app with `SDL_MRCU_TIMER=300000` | R | [jf] window.c 137 to 140 |
| SDL registers the app on the Luna bus at init: it reads `nativeLifeCycleInterfaceVersion` from `luna://com.webos.applicationManager/getAppInfo`, then subscribes to `registerNativeApp` (version 1, messages carry `message`) or `registerApp` (version 2, messages carry `event`) | R | [sdl2] SDL_webos_init.c 66 to 94, 135 to 192 |
| On `relaunch` SDL raises the window (`set_state(3)`); on `close` (version 2 only) it posts `SDL_QUIT`. It logs the message and keeps no payload | R | [sdl2] SDL_webos_init.c 194 to 241, SDL_waylandwindow.c 1627 to 1630 |
| Kodi subscribes to `luna://com.webos.service.applicationmanager/registerApp` with `{}` and goes fullscreen on `relaunch` | R | [kodi] WinSystemWaylandWebOS.cpp 35, 64 to 72, 196 to 209 |
| Registration "is what lets SDL learn about relaunch and close requests, and about screensaver and power state changes" | D | [sdl3] SDL_hints.h 4461 to 4477 |
| With LG's own SDL, a missing `APPID` makes registration fail with "Invalid appId specified" and SDL falls back to plain Wayland | R | [jf] window.c 128 to 134 |
| The SDL3 fork registers in `SDL_Init`, before any window, and unregisters in `SDL_Quit`. With `SDL_WEBOS_REGISTER_APP` off it makes none of its three Luna subscriptions: not the registration, not the screen saver requests, not the power state. Nothing else in SDL reads the power state | R | [sdl3] SDL.c 371 to 386, 748; SDL_webos_init.c 97 to 117, 147 to 160 |
| On `relaunch` the SDL3 fork calls `SDL_RaiseWindow` on its first window, which on webOS is `set_state(3)`: "asking the shell for fullscreen is what brings a backgrounded app back to the front". On `close` (interface 2) it sends `SDL_EVENT_QUIT`. It reads no other field of the message | R | [sdl3] SDL_webos_init.c 222 to 280; SDL_waylandwindow.c 2386 to 2393 |
| Screen saver: the TV asks each registered client before it starts the saver, and SDL answers `ack`, "it may start", false while SDL's screen saver is disabled. `SDL_DisableScreenSaver` and `SDL_EnableScreenSaver` flip that switch and, on webOS, nothing else: the Wayland driver then tries D-Bus and `zwp_idle_inhibit_manager_v1`, which the TV's compositor does not list (section 1). SDL disables the screen saver when video starts unless `SDL_VIDEO_ALLOW_SCREENSAVER` is set, so an SDL app refuses every screen saver by default | R, I | [sdl3] SDL_webos_init.c 307 to 345; SDL_video.c 731 to 733, 4620 to 4658; SDL_waylandwindow.c 3346 to 3381 |
| Surface state in the SDL3 fork: minimized sends `SDL_EVENT_WINDOW_MINIMIZED`, then `SDL_EVENT_WILL_ENTER_BACKGROUND` and `SDL_EVENT_DID_ENTER_BACKGROUND` (`state_about_to_change` sent the first of the two once already); fullscreen after a lower state sends `SDL_EVENT_WINDOW_RESTORED`, `SDL_EVENT_WILL_ENTER_FOREGROUND` and `SDL_EVENT_DID_ENTER_FOREGROUND`, the first fullscreen of a new window included; `close` empties the event queue, then sends `SDL_EVENT_WINDOW_CLOSE_REQUESTED`. Each change also goes to libhelpers (`HProcessAppState`), "the app-state broadcast the media pipeline listens on" | R | [sdl3] SDL_waylandwebos.c 37 to 102; SDL_webos_libs.c 59 to 62 |
| SDL turns SIGINT and SIGTERM into `SDL_EVENT_QUIT` at its next event pump, not when the signal arrives: its handler only sets a flag | R | [sdl3] src/events/SDL_quit.c 47 to 60, 166 to 172 |

## 4. The webOS protocols move between releases

Wayland addresses a request by its position. webOS adds requests in the middle
of an interface and leaves the version at 1 ([brew-wl], **D**). Orders read from the
interface tables dumped from TV firmware 1.2 to 10.2 ([wl-dump], **R**):

| Interface | Same on every dump | Moves |
|-----------|--------------------|-------|
| `wl_webos_shell` | `get_system_pip` 0, `get_shell_surface` 1 | nothing |
| `wl_webos_shell_surface` | `set_location_hint` 0, `set_state` 1, `set_property` 2, all five events | `set_key_mask` 3 from 1.4, `set_size` 4 from 6.2 |
| `wl_webos_input_manager` | event `cursor_visibility` | 1.2: no requests. 1.4 to 3.9: `get_webos_seat` 0. From 4.4: `set_cursor_visibility` 0, `get_webos_seat` 1 |
| `text_model` | requests 0 to 5 (`set_surrounding_text`, `activate`, `deactivate`, `reset`, `set_cursor_rectangle`, `set_content_type`), all twelve events | 1.2 to 2.2: `invoke_action` 6, `commit` 7, `show_input_panel` 8, `hide_input_panel` 9. 3.4 to 3.9: `set_enter_key_type` 6 pushes those to 7, 8, 9, 10. From 4.4: `set_max_text_length` 9, `set_platform_data` 10, show 11, hide 12. From 5.3: `set_input_panel_rect` 13, `reset_input_panel_rect` 14 |
| `wl_webos_foreign`, `wl_webos_exported`, `wl_webos_imported` | | present from 5.3 |

The XML LG publishes for OSE disagrees with every TV dump from 3.4 on: its `text_model` puts
show at 8 and `set_enter_key_type` at 12, its input manager puts `get_webos_seat`
at 0 ([wl-ext] text.xml 325 to 367, webos-input-manager.xml 28 to 49, **R**). Stubs
generated from it call the wrong request on a TV (**I**).

The fix both SDL-webOS and jellyfin-webos-native use: load
`libwayland-webos-client.so.1` from the TV, read `<interface>_interface`, and
find each request's opcode by name ([sdl2] SDL_waylandwebos_abifix.c 49 to 106;
[jf] display.md 80 to 105, **R**). The SDL2 fork does it for the input manager
only and otherwise calls `text_model` requests 1, 2 and 5, which never moved
([sdl2] SDL_waylandvideo.c 972 to 987, **R**).

## 5. SDL2 on the TV

### What each release ships

One firmware per release, read from webosbrew's firmware symbol data ([fw], **R**).
The version column is inferred from the file name, with two anchors: webosbrew
overrides webOS 2 and 3.0 to 3.4 to 2.0.2 ([brew-lib]), and plx-native names
webOS 3.9.3's SDL 2.0.4 ([plx] section 6).

| webOS | Library file | SDL (I) | webOS exports visible in the dump |
|-------|--------------|---------|-----------------------------------|
| 1.2, 1.4 | `libSDL2-2.0.so.0.0.0` | 2.0.0 | none; links `libwayland-webos-client.so.1`, `libwayland-starfish-client.so.1`, `libhelpers.so.2` |
| 2.2, 3.4 | `.so.0.2.1` | 2.0.2 | none |
| 3.9 | `.so.0.4.0` | 2.0.4 | none |
| 4.4, 4.10 | `.so.0.4.1` | 2.0.4 or 2.0.5 | `SDL_webOSCursorVisibility`, `SDL_webOSGetPanelResolution`, `SDL_webOSGetRefreshRate` |
| 5.3, 6.4 | `.so.0.10.0` | 2.0.10 | adds `SDL_webOSCreateExportedWindow`, `SetExportedWindow`, `ExportedSetCropRegion`, `ExportedSetProperty`, `DestroyExportedWindow`; 6.4 adds `SDL_webOSSetInputRegion` |
| 7.4 to 11.2 | `.so.0.14.0`, package `lib32-libsdl2-webos` | 2.0.14 | adds `SDL_webOSSetLSHandle`, `WebOSShowScreenKeyboard`, `WebOSHideScreenKeyboard`, `WAYLAND_text_model_interface` |

| Fact | Tag | Source |
|------|-----|--------|
| LG's SDL has the webOS Wayland backend built in on every release: it links the webOS client library on 1.x and exports the webOS interfaces from 7.x | I | [fw] `needed` and `symbols` of each `libSDL2-2.0.so.0.*.json` |
| LG changed the event layout: `SDL_KeyboardEvent`, `SDL_TextEditingEvent` and `SDL_TextInputEvent` carry an extra `Uint32 inputSource` after `windowID`. A binary built with upstream headers reads the wrong bytes | R | [nc4] 001-webos-input-structs.patch; [sdl2] SDL_events.h 229 to 231; [plx] section 1 |
| The NDK's header for LG's SDL, `SDL_webOS.h`: hints `SDL_WEBOS_ACCESS_POLICY_KEYS_BACK`, `..._EXIT`, `SDL_WEBOS_CURSOR_SLEEP_TIME`, and `SDL_HINT_WEBOS_REGISTER_APP` whose string is `"SDL_HINT_WEBOS_REGISTER_APP"`; scancodes CH_UP 480, CH_DOWN 481, BACK 482, CURSOR_SHOW 484, CURSOR_HIDE 485, RED 486 to BLUE 489, EXIT 505 | R | [nc4] 002-webos-apis.patch |
| The numbers are not stable across firmware: on a webOS 4 set plx-native reads channel 300 and 301, play 450, fast forward 451, rewind 452 from the TV's own table, finds only the BACK and EXIT policies in the binary, and no `SDL_webOSCursorVisibility` on webOS 3.9.3 | R | [plx] sections 1, 2, 5, 6 |

### The webosbrew fork

`webosbrew/SDL-webOS` is upstream SDL plus a webOS Wayland backend, zlib licence
([sdl2] LICENSE.txt). Branches `webOS-2.30.x` (2.30.12, default), `webOS-2.32.x`
and `webOS-3.4.x` (SDL 3.4.16). Latest tags: `release-2.30.12-webos.6`
(2026-08-17, its hotplug monitor "Verified in the app jail on webOS 3.4 (kernel 3.10), 4 (4.4) and 10 (5.4)"),
`release-2.32.10-webos.1`, `release-3.4.16-webos.5` (2026-09-27, "Makes SDL3
start and run on webOS 1", ships `libSDL3.so` and `libSDL3.a`) ([sdl-rel], **D**).
moonlight-tv bundles `release-2.30.12-webos.6` under `lib/backports`
([moonlight] CMakeLists.txt 145 to 149, **R**). The webOS code compiles only with
`-DWEBOS=ON`; without it the tree is stock SDL with X11 and Wayland, and the
webOS scancodes stay in the public enum ([sdl2] CMakeLists.txt 469, 519 to 522,
1762 to 1789; SDL_scancode.h 432 to 463, **R**).

Functions, in `SDL_system.h` under `__WEBOS__` ([sdl2] 630 to 660; same names in
[sdl3] SDL_system.h 834 to 961, **R**):

| Function | Does |
|----------|------|
| `SDL_webOSCursorVisibility(visible)` | `wl_webos_input_manager.set_cursor_visibility`; false where the request does not exist |
| `SDL_webOSGetPanelResolution(&w, &h)` | `luna://com.webos.service.panelcontroller/getPanelResolution`, else 3840x2160 or 1920x1080 from the `UHD` system property |
| `SDL_webOSGetRefreshRate(&rate)` | config `tv.hw.SoCOutputFrameRate`, else 120 when `tv.hw.supportFrc`, else 60 |
| `SDL_webOSCreateExportedWindow(type)`, `SDL_webOSSetExportedWindow(id, src, dst)`, `SDL_webOSExportedSetCropRegion`, `SDL_webOSExportedSetProperty`, `SDL_webOSDestroyExportedWindow` | a punch-through region for the video plane (section 11); types video 0, subtitle 1, transparent 2, opaque 3 |

Hints ([sdl2] SDL_hints.h 2714 to 2770, SDL_waylandwebos.c 73 to 181, 234 to 281,
**R**). The string is the macro name without `HINT_`. A change after the window
exists is forwarded to the surface.

| Hint | Default | Effect |
|------|---------|--------|
| `SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_BACK` | false | "By default, launcher strip will be opened when back key is pressed." True delivers Back |
| `SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_EXIT` | false | Delivers Exit, "long-pressing back for magic remotes" |
| `SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_HOME`, `_GUIDE` | false | Delivers Home ("L_Super on keyboard"), the TV guide key |
| `SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_META` | false | Meta key, "only available on webOS 8 or later" |
| `SDL_HINT_WEBOS_ACCESS_POLICY_RIBBON` | true | False forbids the home launcher ribbon |
| `SDL_HINT_WEBOS_CURSOR_SLEEP_TIME` | 300000 | Milliseconds before the pointer hides: `wl_starfish_pointer.set_mrcu_standby_timer` |
| `SDL_HINT_WEBOS_CURSOR_FREQUENCY` | unset | Pointer update rate, property `cursor_fps` |
| `SDL_HINT_WEBOS_CURSOR_CALIBRATION_DISABLE` | false | Property `restore_cursor_position` |
| `SDL_HINT_WEBOS_REGISTER_APP` | true | False skips the Luna registration of section 3 |
| `SDL_HINT_WEBOS_WINDOW_CLASS` | unset | `"1"` sets `_WEBOS_WINDOW_CLASS` |
| `SDL_HINT_WEBOS_CLOUDGAME_ACTIVE` | true | Property `cloudgame_active`: stops webOS 8.3 and later turning gamepad input into navigation and volume keys |
| `SDL_HINT_WEBOS_HIDAPI_IGNORE_BLUETOOTH_DEVICES` | unset | `0xVVVV/0xPPPP` pairs kept out of HIDAPI |

## 6. System SDL, bundled fork, or raw Wayland

| | System SDL2 | Bundled SDL-webOS | Raw Wayland and EGL |
|---|-------------|-------------------|---------------------|
| Version | 2.0.0 to 2.0.14 by release (section 5) | one version everywhere: 2.30.12 or 3.4.16 | none |
| ABI | LG's event structs and scancodes, different per firmware | upstream ABI; an `-abi` build mimics LG's | own code |
| Protocol drift | handled by LG per firmware | opcode lookup for the input manager; fixed opcodes for `text_model` 0 to 6 | must do the lookup for every moved request |
| Remote keys | BACK and EXIT policies only on old releases | all policies; SDL3 also gives the raw key code | full control |
| On-screen keyboard | visible in exports from 7.x only | SDL2: default content type. SDL3: type, capitalisation, autocorrect, multiline | full `text_model`, opcodes by name |
| Lifecycle, screensaver, power | built in | built in (section 3) | Luna calls by hand |
| Desktop Linux | no | same source builds stock SDL for X11 and Wayland | xdg-shell path to write and test |
| Who ships it | plx-native, jellyfin-webos-native | moonlight-tv, ihsplay | Kodi, RetroArch |

Sources: section 5; [kodi]; [retroarch]; [moonlight]; [ihsplay]; [jf] window.c
11 to 13; [plx]. That the last two run on the TV's own SDL is inferred from the
LG header they include and the LG event layout they decode.

**Recommendation (I): bundle SDL-webOS, the SDL3 line, pinned to
`release-3.4.16-webos.5`.** Reasons, in order of weight:

1. The system SDL is a different library on each release: version, struct layout,
   scancode values and available hints all change. One bundled build removes the
   release from the host's key and text code.
2. The fork already carries the fixes a raw client would rediscover: several
   seats, the opcode lookup, the transparent cursor, the webOS 1 buffer detach
   hang, the `EGL_EXT_present_opaque` break on webOS 10, Luna registration.
3. SDL3 reports every key with `event.key.raw`, the Wayland key code, even when
   it has no scancode ([sdl3] SDL_keyboard.c 548 to 575, SDL_waylandevents.c 2361).
   SDL2 drops a key whose scancode is unknown ([sdl2] SDL_keyboard.c 824 to 826),
   so Info and the pointer show and hide keys never reach an SDL2 app.
4. SDL3's text input maps field types to the keyboard (section 10); SDL2's sends
   one default.
5. The same SDL3 API runs on desktop Linux under Xvfb for the simulator.

Risk: the SDL3 branch had its first release two months before this was written,
and the two apps read here that bundle the fork use its SDL2 line. Fallback: `release-2.30.12-webos.6`, what moonlight-tv
ships, with the SDL2 limits above. Raw Wayland stays the escape hatch for
`text_model` (section 15).

## 7. Remote keys at the lowest level

`wl_keyboard.key` carries the Linux evdev code. The TV's keymap
(`/usr/share/X11/xkb/keycodes/lg`) names it `IR_KEY_*` at code + 8, and its
keysyms are Qt key values above the Unicode keysym range, so `xkb` text lookup
must reject them ([jf] input.md 70 to 72, 91 to 94; [sdl2] SDL_scancode_webos.c
26 to 46, imKStoUCS.c 302 to 307; [qtwl] 668 to 678; [userland], **R**). A bare
number after [sdl2] in this table is a line of `SDL_scancode_webos.c`. The keysym
column is the value LG's keysym header lists; that the TV's keymap emits it for
each key was not checked.

| Button | evdev | XKB, `lg` name | Keysym | Web keyCode | Tag, source |
|--------|-------|----------------|--------|-------------|-------------|
| Left, Up, Right, Down | 105, 103, 106, 108 | | `0x01000012` to `15` | 37, 38, 39, 40 | R [jf] input.md 70; D [lg-remote] |
| OK | 28; RetroArch also takes 352 and 353 | | `0x01000004` | 13 | R [jf]; [retroarch] 884 to 886 |
| Back | 412 | 420 `IR_KEY_BACK` | `0x01200003` | 461 | R [sdl2] 42; [retroarch] h 25; [jf] 91 to 94; D [lg-back] |
| Exit (Back held) | 174 | 182 `IR_KEY_EXIT` | `0x01200001` | none documented | R [sdl2] 32 |
| Home | 773 | 781 `IR_KEY_HOME` | Qt `Key_Super_L` | none | R [sdl2] 43; [glasshouse] 3073 |
| Red, Green, Yellow, Blue | 398, 399, 400, 401 | 406 to 409 | `0x01200011` to `14` | 403 to 406 | R [sdl2] 36 to 39; [retroarch] h 26 to 29; D [lg-remote] |
| Channel up, down | 402, 403 | 410, 411 | `0x01200021`, `22` | 33 for up, as one project saw | R [sdl2] 40 to 41; [glasshouse] 494 |
| Guide | 362 | 370 `IR_KEY_GUIDE` | `0x01200039` | not delivered to a page | R [sdl2] 35; [glasshouse] 487 |
| Info | 358 | 366 `IR_KEY_INFO` | `0x01200002` | 457 | R [glasshouse] 494 |
| Play | 207 | 215 `IR_KEY_PLAY` | `0x01000080` | 415 | R [sdl2] 33; D [lg-remote] |
| Pause | 119 | | `0x01000085` | 19 | R [glasshouse] 1600; D [lg-remote] |
| Stop | 128 | 136 `IR_KEY_STOP` | `0x01000081` | 413 | R [sdl2] 30 |
| Rewind, Fast forward | 168, 208 | 176 `IR_KEY_REW`, 216 `IR_KEY_FF` | `0x010000c5`, `0x01000102` | 412, 417 | R [sdl2] 31, 34 |
| Previous, Next | 813, 814 | 821 `IR_KEY_GOTOPREV`, 822 `IR_KEY_GOTONEXT` | | | R [sdl2] 44 to 45 |
| Pointer shown, hidden | 1198, 1199 | 1206 `IR_KEY_CURSOR_SHOW`, 1207 `IR_KEY_CURSOR_HIDE` | `0x01200201`, `0x01200202` | 1536, 1537 on webOS TV 1.x; later the `cursorStateChange` event | R [sdl3] SDL_scancode_webos.c 44 to 45; [userland]; D [lg-sysui] |
| 0 to 9 | 11, 2 to 10 | | | | I: Linux `KEY_0` to `KEY_9`; no webOS source read confirms it |

## 8. Delivery, repeat, Back and Exit

| Fact | Tag | Source |
|------|-----|--------|
| Back, Exit, Home, Guide and Meta reach an app only once its surface claims them (section 2). Unclaimed, the system acts on them | D | [brew-remote]; [sdl2] SDL_hints.h 2714 to 2737 |
| "The compositor is free to ignore certain keys even if set to '1' ... most commonly this will mean the HOME key" | D | [wl-ext] webos-shell.xml 195 to 196 |
| Power, volume, mute and the AI button are "General system functions". Whether a native app ever sees them is not documented | D | [lg-remote] |
| From webOS 8.3 the TV maps a game controller to navigation and volume keys unless the surface sets `cloudgame_active` | D | [sdl2] SDL_hints.h 2762 to 2769 |
| Repeat, OSE: "auto repeat key events are supposed to be handled by input drivers"; the compositor, when built on Qt 6, sets its own rate to 0 | R | [lsm] weboskeyboard.cpp 26 to 29 |
| Repeat, TV: "Since webOS 7 the compositor sends the following key repeat info: Key repeat rate: 40 cps, delay 400 ms. Which is too fast for the long press detection". Kodi ignores it | D | [kodi] SeatWebOS.cpp 22 to 27 |
| SDL repeats on the client from `wl_keyboard.repeat_info`, and flags any press of a key already down as `repeat` | R | [sdl2] SDL_waylandevents.c 1376 to 1378, 1437 to 1444; [sdl3] SDL_keyboard.c 549 to 557 |
| Before webOS 7: whether the compositor repeats a held remote key is not known | | |
| LG's rule for Back: go to the previous page; on the entry page, "On webOS TV 6.0 or higher, show a popup asking whether to exit the app. On webOS TV 5.0 or lower, launch the Home launcher" | D | [lg-back] |
| LG's checklist: "A click on the Back button on the entry page of the app must display the Home screen (webOS23 - webOS25 only)" | D | [lg-check] |
| Web apps: the platform handles Back through `history` unless `disableBackHistoryAPI` is true, then the page gets keyCode 461 and calls `webOS.platformBack()` to get the system behaviour | D | [lg-back] |
| LG's web app manager implements that by toggling the surface property: `"true"` while the page can go back, `"false"` at the root: "Do not send back key to WAM. LSM should handle it" | R | [wam] web_app_wayland.cc 726 to 733 |
| ihsplay does the same from native code by changing the SDL hint at run time | R | [ihsplay] app_webos.c 11 to 13 |
| OSE's `PlatformBack` is a stub that logs "generate RECENT key". What the TV does for `platformBack` is not public | R | [wam] web_app_wayland_window.cc 80 to 84 |
| moonlight-tv opens Home itself: `luna://com.webos.applicationManager/launch` with `{"id":"com.webos.app.home","params":{"launchType":"homeKey"}}` | R | [moonlight] app_webos.c 85 to 88 |
| Back held is Exit: "Back long press: Exit to Home"; "You can also terminate apps by pressing and holding the Back button" | D | [lg-remote]; [lg-apps] |
| Apps that claim Exit quit on it: moonlight-tv when idle, plx-native always ("terminates the app ... no confirmation") | R | [moonlight] lv_drv_sdl_key.c 241 to 246; [plx] section 2 |

## 9. Magic Remote pointer, wheel, touch

| Fact | Tag | Source |
|------|-----|--------|
| The pointer is an ordinary `wl_pointer`: motion, button, axis. "Nothing special to switch on" | D | [brew-remote] |
| Wheel: SDL divides the axis value by 10, so one notch is 1 | R | [sdl2] SDL_waylandevents.c 78, 688 to 712; [jf] window.c 384 to 393 |
| Two modes: in pointer mode the cursor shows; any arrow key switches to 5-way mode and hides it; shaking the remote switches back | D | [lg-remote] |
| While an overlay such as the launcher shows, "apps cannot receive the pointer events" | D | [lg-remote] |
| The compositor announces visibility two ways. First, `wl_webos_input_manager.cursor_visibility(visibility, webos_seat)`, sent on bind and on every change | R | [lsm] webosseat.cpp 122 to 143; [wl-ext] webos-input-manager.xml 36 to 42 |
| Second, as key presses: evdev 1198 for shown, 1199 for hidden (section 7). The OSE compositor itself toggles the cursor on these keys | R | [lsm] base/src/main.cpp 56 to 67 |
| The SDL2 fork binds the event with an empty listener; the SDL3 fork adds none. The SDL3 fork turns the two keys into `SDL_SCANCODE_WEBOS_CURSOR_SHOW` and `_HIDE`; the SDL2 fork names those scancodes but maps no key to them; LG's SDL delivers 484 and 485 | R | [sdl2] SDL_waylandvideo.c 871 to 879, SDL_scancode_webos.c; [sdl3] SDL_scancode_webos.c 73 to 76; [jf] window.c 98 to 101, 328 to 337 |
| An app hides the pointer with `set_cursor_visibility(0)` (webOS 4 and later). moonlight-tv and jellyfin-webos-native do it on every arrow press | R | [sdl2] SDL_waylandmouse.c 756 to 776; [moonlight] lv_drv_sdl_key.c 268 to 281; [jf] window.c 115 to 122, 348 to 349 |
| Auto-hide: `wl_starfish_pointer.set_mrcu_standby_timer(ms)`. SDL's default is 300000; apps set 5000 or 1000 | R | [sdl2] SDL_waylandwebos.c 78 to 82, SDL_waylandvideo.c 980 to 983; [moonlight] app.c 67; [jf] window.c 141 |
| A null cursor surface "has no effect in webOS", and a blank cursor makes the OSE compositor block all mouse events. SDL sets a 1x1 transparent cursor instead; Kodi never calls `set_cursor`: it "completely breaks pointer input" | R, D | [lsm] webossurfaceitem.cpp 1561 to 1566, base/src/main.cpp 68 to 84; [sdl2] SDL_waylandmouse.c 677 to 686; [kodi] SeatWebOS.cpp 14 to 20 |
| Touch exists on the StanbyME model. `supportTouchMode` in `appinfo.json`: `full` delivers touch events, `virtual` turns them into mouse and key events, `none` (default) delivers nothing | D | [lg-touch] |

## 10. Text input and the on-screen keyboard

The protocol is `text_model_factory.create_text_model` and the `text_model` it
returns ([wl-ext] text.xml 30 to 400, **D**). Indexes are bytes.

| Request | Use |
|---------|-----|
| `activate(serial, seat, surface)` | the field takes the keyboard |
| `deactivate(seat)` | the field gives it up |
| `set_content_type(hint, purpose)` | hint bits: completion 0x1, correction 0x2, capitalisation 0x4 (default 0x7), lowercase 0x8, uppercase 0x10, titlecase 0x20, hidden text 0x40, sensitive 0x80 (password 0xc0), latin 0x100, multiline 0x200. Purpose: normal 0, alpha 1, digits 2, number 3, phone 4, url 5, email 6, name 7, password 8, date 9, time 10, datetime 11, terminal 12 |
| `set_enter_key_type(type)` | default 0, return 1, done 2, go 3, send 4, search 5, next 6, previous 7. Absent on webOS 1 and 2 |
| `set_surrounding_text(text, cursor, anchor)`, `reset(serial)`, `commit` | context for prediction, drop the composition, apply |
| `show_input_panel`, `hide_input_panel`, `set_max_text_length`, `set_platform_data`, `set_input_panel_rect` | opcodes differ per release (section 4) |

| Event | Meaning |
|-------|---------|
| `enter(surface)`, `leave` | the model became active; it was deactivated or the surface lost focus |
| `commit_string(serial, text)` | insert text, replacing any composition |
| `preedit_string(serial, text, commit)`, after `preedit_styling` and `preedit_cursor` | composition text to draw at the caret |
| `delete_surrounding_text(serial, index, length)` | delete `length` bytes at `index` from the caret |
| `keysym(serial, time, sym, state, modifiers)` | a panel key that is not text |
| `input_panel_state(state)` | 1 shown, 0 hidden |
| `input_panel_rect(x, y, w, h)` | panel geometry |

| Fact | Tag | Source |
|------|-----|--------|
| LG's own clients show the panel by creating a model and calling `activate`; they send content type, enter key type and surrounding text once `enter` arrives | R | [qtwl] 402 to 407, 711 to 723; [chromium] 126 to 143, 223 to 262 |
| "LG's compositor raises the input panel on activate; show_input_panel is never sent by its own clients and does not raise it" | D | [sdl3] SDL_waylandwebos_osk.c 132 to 133 |
| The compositor refuses `activate` for a surface without keyboard focus and answers `leave`. Focus "arrives about a second after the first buffer is committed" | R, D | [lsm] waylandtextmodel.cpp 191 to 197; [sdl3] osk 276 to 278 |
| `deactivate` destroys the model on the compositor side: every show needs a new one. "There is no way to show already activated model" | R | [lsm] waylandtextmodel.cpp 214 to 224; [qtwl] 739 to 745 |
| `delete_surrounding_text` with length `UINT_MAX` is the panel's Clear all | R | [qtwl] 589 to 592; [sdl3] osk 378 to 383 |
| `keysym` carries Enter and the panel's arrows as key symbols. Qt turns Return into `"\r"`; Chromium forwards them as key events and hides the panel on Tab, or on `KEY_PREVIOUS` (412, the Back key), Up or Down once the panel is hidden | R | [qtwl] 659 to 707; [chromium] 331 to 378 |
| Password: hint 0xc0 with purpose 8 | R | [chromium] 41 to 62 |
| `input_panel_rect` reports a size with no origin: y is 0 though the panel is docked at the bottom | D | [sdl3] osk 494 to 496 |
| The panel covers the full width and a third of the height, from the bottom | D | [lg-vkb] |
| A hardware keyboard: "if the system detects a keypress from a USB keyboard, the virtual keyboard disappears". The model stays active and keeps delivering text | D | [lg-vkb]; [sdl3] osk 485 to 488 |
| A hardware keyboard's keys arrive on `wl_keyboard` with an xkb keymap; SDL turns them into key events and text | R | [sdl2] SDL_waylandevents.c 1336 to 1379 |
| SDL2 fork: `SDL_StartTextInput` creates a model, activates it and sets hint 0x7, purpose 0, nothing else. Commit becomes `SDL_TEXTINPUT`, preedit `SDL_TEXTEDITING`, Clear all a press of `SDL_SCANCODE_CLEAR`, `keysym` a key event; panel state 0 deactivates and disables text events | R | [sdl2] SDL_waylandwebos_osk.c 84 to 125, 158 to 275 |
| SDL3 fork: `SDL_StartTextInputWithProperties` maps `SDL_PROP_TEXTINPUT_TYPE_NUMBER` to a purpose (text 0, name 7, e-mail 6, password 8 with 0xc0, number 3, PIN 2), capitalisation and autocorrect to hints, multiline to hint 0x200 and enter key return, single line to enter key done | R | [sdl3] osk 204 to 282 |
| SDL3 fork events: `SDL_EVENT_TEXT_INPUT`, `SDL_EVENT_TEXT_EDITING` with the preedit cursor, `delete_surrounding_text` replayed as Backspace and Delete presses, Clear all as `SDL_SCANCODE_CLEAR`, `SDL_EVENT_SCREEN_KEYBOARD_SHOWN` and `_HIDDEN` from the panel state | R | [sdl3] osk 330 to 405, 431 to 490 |
| SDL3 fork gaps: no surrounding text, no url or phone purpose, enter key done or return only, and it sends `set_enter_key_type` at opcode 6, which is `invoke_action` on webOS 1 and 2 | R, I | [sdl3] osk 136 to 138; section 4 |
| SDL3 fork: starting text input on a window where it is already started records the new properties and does nothing else: `StartTextInput` runs only `if (!window->text_input_active)`. New properties, or the panel again, take a stop and then a start | R | [sdl3] SDL_video.c 5798 to 5838 |
| SDL3 fork: the panel has no hide of its own. `HideScreenKeyboard` is `StopTextInput`, which deactivates and destroys the model | R | [sdl3] osk 96 to 117, 284 to 292, 325 to 328 |
| SDL3 fork: that stop sends no `SDL_EVENT_SCREEN_KEYBOARD_HIDDEN`, since the model that would report `input_panel_state` is gone, and SDL's own shown flag stays set. `SDL_SendScreenKeyboardShown` returns early while the flag is set, so the panel's next raise sends no `SDL_EVENT_SCREEN_KEYBOARD_SHOWN`; `SDL_ScreenKeyboardShown` answers true from the stop on, until the compositor reports the panel hidden | R, I | [sdl3] osk 96 to 117, SDL_video.c 5847 to 5866, 5919 to 5956 |
| SDL3 fork: a commit empties an active preedit first, as two events: `SDL_EVENT_TEXT_EDITING` with an empty text, then `SDL_EVENT_TEXT_INPUT`. A preedit the user deleted is the empty editing event alone | R | [sdl3] osk 330 to 364 |
| SDL3 fork: `SDL_EVENT_TEXT_EDITING`'s `start` is the preedit cursor in characters, -1 when the compositor sent none; `length` is -1 | R | [sdl3] osk 345 to 364 |
| SDL sends text and editing events only while text input is started and a window has keyboard focus. A hardware key's text follows its key down as a text event, unless Ctrl or Alt is down | R | [sdl3] SDL_keyboard.c 764 to 770, 817 to 823; SDL_waylandevents.c 2321 to 2325, 2362 to 2369 |
| SDL3 fork: `SDL_SetTextInputArea(rect, cursor)` becomes `set_cursor_rectangle(min(x + cursor, x + w), y, 1, h)`; `SDL_ClearComposition` becomes `reset` | R | [sdl3] osk 68 to 94, 306 to 316 |
| SDL3 fork: `SDL_HasScreenKeyboardSupport` is true whenever the compositor has the text model factory: "The remote is advertised as a keyboard, but the input panel is the only way to enter text with it". On X11 it is true only with Steam's keyboard | D, R | [sdl3] SDL_waylandkeyboard.c 288 to 305; src/video/x11/SDL_x11keyboard.c 844 to 848 |
| LG's SDL: screen keyboard symbols show in the dump from webOS 7. Older builds export the public API only, so nothing can be said | I | [fw] |

## 11. EGL and OpenGL ES

| Fact | Tag | Source |
|------|-----|--------|
| The EGL display comes from the `wl_display`: SDL calls `eglGetDisplay` with it; Kodi creates an `EGL_EXT_platform_wayland` platform display. The surface is a `wl_egl_window` | R | [sdl2] SDL_waylandopengles.c 38 to 49, SDL_waylandwindow.c 2100 to 2111; [kodi] WinSystemWaylandEGLContext.cpp 23, 35, 67 |
| `libEGL.so.1`, `libGLESv2.so.2` and `libwayland-client.so.0` resolve on every dump. `libwayland-egl` is `.so.0` up to webOS 4.4 and `.so.1` from 4.10. The GPU library differs: PowerVR on the 1.x, 2.2 and 10.2 dumps (`libIMGegl`, on 10.2 `libGLESv2_PVR_MESA`), `libmali` on the others | R | [fw] `index.json` of each firmware |
| OpenGL ES 2.0 everywhere: `libGLESv2.so.2` is on every dump and Kodi asks for an ES 2 context on every TV | R, I | [fw]; [kodi] WinSystemWaylandEGLContextGLES.cpp 41, 81 |
| ES 3: `glBindVertexArray` is exported on the webOS 1.4 dump and, with ES 3.1 and 3.2 entry points, on the 10.2 dump; the 4.4 dump's `libmali` exports ES 2 entry points without it; other dumps list no GL symbols. One TV with a Mali-G52 reports "OpenGL ES 3.2" and EGL 1.5 | R | [fw]; [jf] opengl.md 12 to 20 |
| No table of ES version by model year was found | | |
| Alpha matters: "On webOS, we need alpha channels to properly composite the window with the video behind". SDL defaults to RGBA 8888, depth 16, stencil 0. "The TV's video plane shows through wherever the UI writes alpha 0" | R, D | [sdl2] SDL_video.c 3653 to 3672; [jf] window.c 163 to 164 |
| The graphics plane is 1920x1080 on UHD models and 1280x720 on FHD models; video plays at 3840x2160 and 1920x1080. "The reported resolution is always 1080p even for 4K devices and 720p for HD devices" | D | [lg-res]; [kodi] WinSystemWaylandWebOS.cpp 211 to 235 |
| The size is a per-model compositor setting, `com.webos.surfacemanager.compositorGeometry`, default `1920x1080+0+0r0s1`. One project found 3840x2160 configured for four other device layers (`e60n`, `o228k`, `o22n28k`, `o22n8k`), and no client request to change it | R | [lsm] surface-manager.sh.in 56; [jf] display.md 128 to 164 |
| The plane runs at 60 Hz whatever the panel does: `wl_output.mode` reports 1920x1080 at 60000 mHz on a 120 Hz TV | R | [jf] opengl.md 93 to 107 |
| Swap: SDL sets `eglSwapInterval(0)` and paces itself on `wl_surface.frame` with a timeout. With interval 1 one project measured 16.66 ms a frame, with 0 it measured 2.68 ms | R | [sdl2] SDL_waylandopengles.c 60 to 92, 137 to 195; [jf] opengl.md 123 to 130 |
| Video under the UI, webOS 5 and later: `wl_webos_foreign.export_element(surface, type)` returns a `wl_webos_exported` whose `window_id_assigned` gives the id to hand to the media pipeline; `set_exported_window(source, destination)` places it. webOS 4 uses `libAcbAPI` instead | D, R | [brew-media]; [wl-ext] webos-foreign.xml 22 to 104; [kodi] WinSystemWaylandWebOS.cpp 60 to 62, 97 to 108 |
| webOS 1: attaching a null buffer makes `eglSwapBuffers` hang forever | D | [sdl2] SDL_waylandwindow.c 1287 to 1291 |
| webOS 10 and later report `EGL_EXT_present_opaque`; "composition breaks if we use it" | D | [sdl2] SDL_waylandopengles.c 1220 to 1227 |
| webOS 9: "unpopulated jailer config could cause graphics driver initialization failure"; window creation fails | D | [moonlight] root.c 307 to 313 |
| Mali r46p0 advertises `GL_EXT_disjoint_timer_query` and never completes a query | R | [jf] opengl.md 71 to 75 |

## 12. Audio

Out of scope. Apps use SDL audio, which reaches PulseAudio (`libpulse.so.0` is on
every dump, [fw] **R**); low-latency media goes through NDL DirectAudio
([brew-media] **D**).

## 13. Open questions

- Does an EGL config with an 8-bit stencil exist on each GPU family? Skia wants one.
- Which model years expose ES 3.0? Only two firmware dumps and one TV answer.
- What does a native app call for `platformBack`: is leaving Back unclaimed at the
  root enough to get the exit popup (6.0 and later) and the Home launcher (5.0
  and earlier), as it is for LG's web app manager?
- May a store or developer mode app call `applicationManager/launch` for
  `com.webos.app.home`, or does it answer -104, "Cannot launch a privileged app
  on development mode from developer apps path"? With Home shown that way, is
  the app left running behind it, and does LG's QA accept that for "must display
  the Home screen" on webOS 23 to 25?
- Does the TV's app manager deliver `relaunch`, `pause` and `close` to the host's
  own registration as OSE's does, and is `tvpower`'s screen saver request
  reachable without SDL's registration?
- How long does the host take to stop on a TV after `close` or SIGTERM? The
  simulator stops React Native in 16 to 41 ms; the TV allows one second.
- Does the compositor deliver Back to the app or to the keyboard while the panel
  is up?
- Does a second activation raise the panel while a USB keyboard is attached?
  The host takes SDL's stale shown flag for the panel after a stop and a start.
- Do the remote's digit keys type into a focused field, as text events, while
  the panel is up or after it was closed?
- Do the digit, Info, Record and subtitle keys arrive at a native surface without
  a key mask, and with which evdev codes on each remote?
- Before webOS 7, does a held remote key repeat, and at what rate?
- Is `set_cursor_visibility` honoured while the user moves the remote?
- Which webOS release first sent the pointer keys 1198 and 1199 to native surfaces?
- The environment the TV's app manager really sets, beyond `APPID`.
- Does the SDL3 fork's `set_enter_key_type` harm webOS 1 and 2, if they are targets?
- LG's Flutter embedder for webOS ships as a prebuilt artifact ([lg-flutter]
  webos_cache.dart 136); its window and key code could not be read.

## 14. Sources

Read 2026-11-11. Clones are pinned by commit.

| ID | Source |
|----|--------|
| sdl2 | webosbrew/SDL-webOS, branch `webOS-2.30.x` at `ba4c86a`: https://github.com/webosbrew/SDL-webOS/tree/ba4c86af391d1524d3ff0cb61dc3b10035933239 (`include/`, `src/video/wayland/`, `src/events/`, `src/core/webos/`, `wayland-protocols/`) |
| sdl3 | the same repository, branch `webOS-3.4.x` at `f599b85`: https://github.com/webosbrew/SDL-webOS/tree/f599b850f9b1347cebdbb9a806999ddd170ab661 |
| sdl-rel | its releases and their notes: https://github.com/webosbrew/SDL-webOS/releases |
| wl-ext | webosose/webos-wayland-extensions at `4b24337`, `protocol/*.xml`: https://github.com/webosose/webos-wayland-extensions/tree/4b243378b451bd139466c242ae1e58753fea0fbf/protocol |
| wl-dump | webosbrew/wayland-protocols at `87f3ebd`, `protocols/dumped/tv-*/webos.xml`: https://github.com/webosbrew/wayland-protocols/tree/87f3ebdb5c5cc7cfb9d6dbb1e59c4cd7ab2fd74a |
| lsm | webosose/luna-surfacemanager at `b2fef6f`: https://github.com/webosose/luna-surfacemanager/tree/b2fef6f73232244f6f13102aa66d2d3f5942835e |
| wam | webosose/wam at `a486711`, `src/platform/web_app_wayland.cc`, `web_app_wayland_window.cc`: https://github.com/webosose/wam/tree/a486711e2f093723609bd76d0566bf05edae1fcb/src/platform |
| qtwl | webosose/qtwayland-webos at `12cdb7b`, `src/plugins/platforminputcontexts/wayland/waylandinputcontext.cpp`: https://github.com/webosose/qtwayland-webos/blob/12cdb7bae35ef423f7abeffec4981ef0ae0b2c58/src/plugins/platforminputcontexts/wayland/waylandinputcontext.cpp |
| chromium | webosose/chromium108 at `431ed1c`, `src/ozone/wayland/input/webos_text_input.cc`: https://github.com/webosose/chromium108/blob/431ed1c647b24f2337a5883e8011a0c4da73e710/src/ozone/wayland/input/webos_text_input.cc |
| sam | webosose/sam at `96161d3`, `src/bus/client/NativeContainer.cpp`: https://github.com/webosose/sam/blob/96161d3e50670fb21daccfb12a75c35fe406ce0e/src/bus/client/NativeContainer.cpp |
| userland | webosbrew/webos-userland at `00a5f87`, `include/xkbcommon/xkbcommon-webos-keysyms.h`: https://github.com/webosbrew/webos-userland/blob/00a5f8721e15d5c10030b99b2f3e73e0c527bcce/include/xkbcommon/xkbcommon-webos-keysyms.h |
| nc4 | openlgtv/buildroot-nc4 at `322ff04`, `package/sdl2/webos/*.patch`: https://github.com/openlgtv/buildroot-nc4/tree/322ff04e625a7927338c2ac5f3d02cd0511c9b90/package/sdl2/webos |
| fw | webosbrew/dev-toolbox-cli at `2d5a7b6`, `common/data/<firmware>/`: https://github.com/webosbrew/dev-toolbox-cli/tree/2d5a7b6881850d825aa7fdc6c0b5e53c91dc40d3/common/data |
| kodi | xbmc/xbmc at `383db99`, `xbmc/windowing/wayland/`: https://github.com/xbmc/xbmc/tree/383db99b78d5c81453fa3fd5783334b17d9ac984/xbmc/windowing/wayland |
| retroarch | libretro/RetroArch at `57e45b0`, `input/common/wayland_common_webos.c` and `.h`: https://github.com/libretro/RetroArch/blob/57e45b09cb881e2e517e4aa098660011e284e0ae/input/common/wayland_common_webos.c |
| moonlight | mariotaku/moonlight-tv at `cf96ecc`: https://github.com/mariotaku/moonlight-tv/tree/cf96ecc72cdab3aae7ba0698f463b89065ae7e3b |
| ihsplay | mariotaku/ihsplay at `3b38260`: https://github.com/mariotaku/ihsplay/tree/3b382603a1238b96a5162112180edf8c990aed34 |
| jf | HookedBehemoth/jellyfin-webos-native at `c64e6ba`, `docs/input.md`, `display.md`, `opengl.md`, `src/platform/window.c`: https://github.com/HookedBehemoth/jellyfin-webos-native/tree/c64e6ba0db5c9f90ee47f17abf764ae69112c104 |
| plx | GLinnik21/plx-native at `f8f6dbb`, `docs/remote-keys.md`: https://github.com/GLinnik21/plx-native/blob/f8f6dbb75c783de7c790608653e212a14c2ec66c/docs/remote-keys.md |
| glasshouse | rorygallagher2024/glasshouse at `567a177`, `docs/development/TV-RESEARCH.md` (C2 on webOS 9.2.2, C3 on 10.2.2, B8): https://github.com/rorygallagher2024/glasshouse/blob/567a17796de50052091f95524e02c455ce5b6b21/docs/development/TV-RESEARCH.md |
| lg-flutter | lg-flutter-webos/flutter-webos at `9503981`: https://github.com/lg-flutter-webos/flutter-webos/tree/950398131830e9bd5dc91e588c3db8f5c42322be |
| brew-wl, brew-remote, brew-gamepad, brew-media, brew-lib | webosbrew.org guides, source at `fc4b8aa` under `src/views/develop/guides/` (`wayland.md`, `native/remote.md`, `native/gamepad.md`, `native/media.md`, `library-version.md`): https://www.webosbrew.org/develop/guides/wayland/ and its siblings `native/remote`, `native/gamepad`, `native/media`, `library-version/`; source https://github.com/webosbrew/webosbrew.github.io/tree/fc4b8aaa23cbb4784f8eabc540a778d44965b3ab/src/views/develop/guides |
| lg-back | LG, Back Button: https://webostv.developer.lge.com/develop/guides/back-button |
| lg-remote | LG, Magic Remote: https://webostv.developer.lge.com/develop/guides/magic-remote |
| lg-sysui | LG, System UI Visibility: https://webostv.developer.lge.com/develop/guides/system-ui-visibility |
| lg-vkb | LG, Virtual Keyboard: https://webostv.developer.lge.com/develop/guides/virtual-keyboard |
| lg-res | LG, App Resolution: https://webostv.developer.lge.com/develop/specifications/app-resolution |
| lg-check | LG, App Self Checklist: https://webostv.developer.lge.com/distribute/app-self-checklist |
| lg-touch | LG, Touch Screen (StanbyME): https://webostv.developer.lge.com/develop/guides/stanbyme-touch-screen |
| lg-apps | LG, Interacting with Apps: https://webostv.developer.lge.com/develop/guides/interacting-with-apps |
| rn-host | this repository, `runtime/host/src/input/RemoteKeyMap.cpp`, `RemotePresses.h`, `ImeSession.h` |

## 15. What this means for react-native-webos

All of this section is **I**: a design drawn from the facts above, to be checked on
a TV.

### Stack

- Window, GL context, events: SDL 3 from `webosbrew/SDL-webOS`, tag
  `release-3.4.16-webos.5`, built with `-DWEBOS=ON` and linked into the runtime.
  Never the TV's `libSDL2`.
- Before `SDL_Init`: `SDL_WEBOS_ACCESS_POLICY_KEYS_BACK` true,
  `SDL_WEBOS_CURSOR_SLEEP_TIME` 5000. Leave EXIT, HOME, GUIDE and META unclaimed
  and RIBBON allowed. `APPID` must be set: abort with a clear message when it is
  not, as during an ssh run.
- Window: fullscreen at the display mode SDL reports, 1920x1080 on UHD and
  1280x720 on FHD; the logical 1920x1080 screen scales to it. Never size from
  `SDL_webOSGetPanelResolution`.
- Context: ask for ES 3.0, fall back to ES 2.0; RGBA 8888, stencil 8 with a
  fallback to 0. Clear to alpha 1 unless a video hole is wanted. Swap interval 1.
- Simulator: the same host on stock SDL 3 under Xvfb. The webOS scancodes exist in
  the enum on every platform; `event.key.raw` is an evdev code on webOS only.

### Remote keys

Match on the scancode; fall back to `event.key.raw` where the table says raw.
Event names are those `RemoteKeyMap.cpp` already emits ([rn-host]).

| Remote key | evdev (`raw`) | SDL3 fork scancode | SDL2 fork scancode | Host event |
|------------|---------------|--------------------|--------------------|------------|
| Up, Down, Left, Right | 103, 108, 105, 106 | `UP` 82, `DOWN` 81, `LEFT` 80, `RIGHT` 79 | same | `up` `down` `left` `right`, held `longUp` `longDown` `longLeft` `longRight` |
| OK | 28, also 96, 352, 353 | `RETURN` 40, `KP_ENTER` 88; 352 and 353 raw | 40, 88; 352 and 353 dropped | `select`, held `longSelect` |
| Back | 412 | `WEBOS_BACK` 367 | 482 | Back: `onRequestClose` of the last Modal, else `hardwareBackPress` |
| Escape (keyboard, simulator) | 1 | `ESCAPE` 41 | 41 | Back |
| Exit | 174 | `WEBOS_EXIT` 375 | 505 | none: unclaimed, the system closes the app |
| Home | 773 | `WEBOS_HOME` 364 | 384 | none: unclaimed |
| Play | 207 | `MEDIA_PLAY` 262 | `AUDIOPLAY` 261 | `play` |
| Pause | 119 | `PAUSE` 72 | 72 | `pause` |
| Play/Pause (keyboards) | 164 | `MEDIA_PLAY_PAUSE` 271 | `AUDIOPLAY` 261 | `playPause`, held `longPlayPause` |
| Stop | 128 | `MEDIA_STOP` 269 | `AUDIOSTOP` 260 | `stop` |
| Rewind, Fast forward | 168, 208 | `MEDIA_REWIND` 266, `MEDIA_FAST_FORWARD` 265 | `AUDIOREWIND` 285, `AUDIOFASTFORWARD` 286 | `rewind`, `fastForward` |
| Previous, Next | 813, 814 | `MEDIA_PREVIOUS_TRACK` 268, `MEDIA_NEXT_TRACK` 267 | `AUDIOPREV` 259, `AUDIONEXT` 258 | `previous`, `next` |
| Record | 167 | `MEDIA_RECORD` 264 | dropped | `record` |
| Channel up, down | 402, 403 | `WEBOS_CH_UP` 365, `WEBOS_CH_DOWN` 366 | 480, 481 | `channelUp`, `channelDown` |
| Guide | 362 | `WEBOS_GUIDE` 374, only if claimed | 495 | `guide`; not claimed by default |
| Info | 358 | none: raw 358 | dropped | `info` |
| Red, Green, Yellow, Blue | 398 to 401 | `WEBOS_RED` 370 to `WEBOS_BLUE` 373 | 486 to 489 | `red` `green` `yellow` `blue` |
| 0 to 9 | 11, 2 to 10 | `0` 39, `1` to `9` 30 to 38 | same | `0` to `9` |
| Backspace (keyboard) | 14 | `BACKSPACE` 42 | 42 | `delete` |
| Pointer shown, hidden | 1198, 1199 | `WEBOS_CURSOR_SHOW` 368, `_HIDE` 369 | dropped | no TV event: switch between pointer and focus mode |

`menu`, `captions` and `voice` have no webOS key identified yet: leave them out.
`RemotePresses` keeps its contract: a press is one event at release, a hold is
`long<Type>` 0 then 1, and a key down that carries SDL's `repeat` flag, or for a
key already down, is never a new press. The 400 ms repeat delay of webOS 7 sits
above the 300 ms hold timer, so the timer, not the repeat, starts a hold.

### Back at the root

Back is claimed for as long as the app runs. When no `BackHandler` listener
takes a press, React Native calls `exitApp`, which is also what
`BackHandler.exitApp()` calls: the host cannot tell the two apart, and answers
both in one function, `leaveOnBackAtRoot` (`platform/BackAtRoot.h`).

| Behaviour | What it does | For | Against |
|-----------|--------------|-----|---------|
| `ShowHome`, the default | Asks the app manager to launch `com.webos.app.home` with `{"launchType":"homeKey"}`, as moonlight-tv does; the app keeps running behind Home. Ends the app when the bus is missing or the call is refused | LG's checklist: Back on the entry page "must display the Home screen (webOS23 - webOS25 only)". It is what the web platform did on webOS 5 and earlier | Whether an app outside homebrew may launch Home is not known (section 13). LG's Back guide asks for an exit popup on webOS 6 and later, which this does not show |
| `Exit` | Ends the app | Needs nothing from the TV | The TV then shows what it shows when an app closes, which may be the last input and not Home |
| Hand Back to the system: `SDL_WEBOS_ACCESS_POLICY_KEYS_BACK` false at the root, the way LG's web app manager does it | The system answers Back itself: exit popup on webOS 6 and later, Home before | It is LG's own mechanism, and the only one that can show the system's exit popup | Not built: React Native tells the host only after a press went unhandled, so the system would answer the next press, not this one. It needs the app to say when it is at its root. On webOS 26 a Flutter app, which cannot claim Back, gets the exit popup on every press (`webos-tv-platform.md`, section 3.5) |

A TV must confirm, for each webOS generation the package supports: that
`launch` of Home succeeds from an app installed in Developer Mode and from a
store app, that the app is alive behind Home afterwards, and which of the
behaviours LG's QA accepts. Until then `kBackAtRoot` is the one line to change.

### Lifecycle

- **The host registers the app, SDL does not** (`SDL_WEBOS_REGISTER_APP` off,
  `platform/AppRegistration.h`). The app manager keeps one registration per app
  and OSE's two generations disagree on a second one: the first replaces the
  earlier call without telling it, today's refuses the later call
  (`webos-tv-platform.md`, section 3.3). A second subscription beside SDL's
  would therefore either silence SDL or never hear anything, and which one a
  TV does cannot be read anywhere. One registration, made by the host, has
  neither problem, and what SDL did with its own is four small things the host
  now does: raise the window on `relaunch`, quit on `close`, answer the screen
  saver requests, and nothing with the power state.
- Registration goes out before the window opens, well inside the three seconds
  the app manager gives. The interface is the one the launch argument names,
  else 2, which `rn-webos` writes into `appinfo.json`; when the app manager
  answers that the app declares the other one, the host registers again under
  it, once.
- Relaunch: the host raises the window (`SDL_RaiseWindow`), keeps the
  parameters for `getLaunchParameters`, emits `webOSRelaunch` with them, and
  emits `url` when they carry a `contentTarget`. `Linking.getInitialURL` stays
  the deep link of the first launch.
- Background and foreground: `SDL_EVENT_DID_ENTER_BACKGROUND` and the app
  manager's `pause` both pause the app (`AppState` background, every held key
  released, players that may not play hidden paused);
  `SDL_EVENT_DID_ENTER_FOREGROUND` resumes it. A relaunch does not resume by
  itself: the surface's return to fullscreen does. `SDL_EVENT_WINDOW_FOCUS_LOST`
  releases held keys without pausing.
- Leaving: the app manager's `close`, `SDL_EVENT_QUIT` and
  `SDL_EVENT_WINDOW_CLOSE_REQUESTED` end the main loop, and the host stops
  React Native and returns from `main`. SIGTERM and SIGINT are blocked in every
  thread and taken by one that waits for them (`platform/TerminationSignals.h`),
  so the main loop ends at once instead of at SDL's next event pump; a second
  signal ends the process. Exit and Home are never claimed: a held Back closes
  the app from outside.
- Screen saver: allowed (`SDL_VIDEO_ALLOW_SCREENSAVER`): it is what keeps a
  still image from marking the panel. The host answers the TV's requests
  itself (`platform/ScreenSaver.h`) from SDL's switch, which expo-keep-awake
  flips with `SDL_DisableScreenSaver` and `SDL_EnableScreenSaver`.

### Text input

Built as below in `runtime/host/src/input/ImeSession.*`, `ImeOptions.cpp`,
`KeyboardFocus.*` and `views/TextInput*`; [text-input.md](text-input.md) has the
field's own contract.

- `ImeSession` wraps SDL: `begin` calls `SDL_StartTextInputWithProperties`, `end`
  calls `SDL_StopTextInput`. Lowering the panel for a field that keeps the
  keyboard is a stop too, and raising it again a stop and a start, since SDL
  has no other way (section 10).
- Props to SDL properties:

  | Prop | `SDL_PROP_TEXTINPUT_*` | Value |
  |------|------------------------|-------|
  | `keyboardType` | `TYPE_NUMBER` | `email-address`: `TEXT_EMAIL`; `visible-password`: `TEXT_PASSWORD_VISIBLE`; `numeric`, `number-pad`, `decimal-pad`: `NUMBER`; anything else: `TEXT` |
  | `secureTextEntry` | `TYPE_NUMBER` | `TEXT_PASSWORD_HIDDEN`, or `NUMBER_PASSWORD_HIDDEN` on a number keyboard, as Android's numeric password; it wins over the other text types |
  | `autoCapitalize` | `CAPITALIZATION_NUMBER` | `none`: `NONE`; `words`: `WORDS`; `characters`: `LETTERS`; anything else: `SENTENCES`. `NONE` on a secure field |
  | `autoCorrect` | `AUTOCORRECT_BOOLEAN` | the prop; false on a secure field |
  | `multiline` | `MULTILINE_BOOLEAN` | the prop |

- Not honoured, each said once in the log when a field with it takes the
  keyboard: `keyboardType` `url` and `phone-pad` (SDL has no type: the text
  keyboard opens); a `returnKeyType` other than `done` on a single line (the
  fork sets Done, and Return on a multiline field); `showSoftInputOnFocus={false}`
  (activating the model raises the panel). The three number keyboards are one
  SDL type: whether the panel shows a sign or a decimal point is its own choice.
- `SDL_EVENT_TEXT_INPUT` is `imeCommit`; `SDL_EVENT_TEXT_EDITING` is
  `imePreedit`. An empty editing event waits for the events queued with it: when
  a commit follows, the field sees the commit alone, one edit as on Android;
  otherwise the composition ends.
- The Backspace, Delete and Clear presses SDL synthesises go to the field's own
  key handling (Clear empties the field); a Return press submits a single-line
  field.
- Whether the panel is up comes from `SDL_EVENT_SCREEN_KEYBOARD_SHOWN` and
  `_HIDDEN`, and after a start from `SDL_ScreenKeyboardShown`: once the host
  has lowered the panel by a stop, SDL sends no shown event for the next raise
  (section 10), so its stale flag is the only sign that the panel is up again.
  `SDL_EVENT_SCREEN_KEYBOARD_HIDDEN` is `imePanelHidden`.
- While a field has the keyboard, the field sees every key before
  `RemotePresses`. Back lowers the panel when it is up, and blurs the field
  otherwise. OK raises the panel again when the user closed it. A key that
  types means a hardware keyboard, which lowers the panel by itself
  (section 10): after one, OK submits instead.
- A key whose keycode is a printable character is the field's while text input
  is started, so a digit typed into a field is never also a remote key; its
  text comes in the text event that follows. After Back lowered the panel, no
  text arrives until OK raises it again.
- If surrounding text, the missing purposes or the enter key label turn out to
  matter, the host talks to `text_model` itself on SDL's connection
  (`SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER`, `..._SURFACE_POINTER`), with every
  opcode looked up by name in the TV's `libwayland-webos-client.so.1`.

### Pointer

`input/PointerInput` turns SDL's mouse events into what React Native has for a
pointer: W3C pointer enter, leave and move events on the views under it (the
feature flag `shouldPressibilityUseW3CPointerEventsForHover` makes `Pressable`
call `onHoverIn` and `onHoverOut` from them), touches for the button (the
responder system makes presses of them), and a scroll of the nearest scroll
view for the wheel, 100 points a notch. The pointer leaving
(`SDL_SCANCODE_WEBOS_CURSOR_HIDE`, the window losing the pointer or the focus)
ends the hover and cancels a press. The system draws the cursor and hides it
when an arrow key is pressed (section 9), so the host draws none and hides
none. Verified in the simulator with injected mouse events; on a TV, the
events the Magic Remote really sends remain to be seen.
