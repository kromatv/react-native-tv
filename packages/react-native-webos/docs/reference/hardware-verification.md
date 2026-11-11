# What has run on a TV

Every claim of "verified on hardware" in this package points here: which TV,
which build, what was pressed and what the screen showed. A row is what was
seen, once, on that TV. It says nothing about other chipsets or webOS versions.

## How the TVs were reached

LG's webOS Cloud Test Lab, in Seller Lounge
([webos-tv-platform.md](webos-tv-platform.md), section 1.4): real TVs in Korea,
shown as a 1280x720 video at about 10 frames a second, with a virtual remote.
The lab installs the uploaded `.ipk` itself. It gives no shell and no log, so
nothing here comes from `rn-webos run`, `logs`, `perf` or `screenshot`, and no
frame rate was measured: the video is too slow to judge one.

Screenshots are frames of that video, kept outside the repository.

## TV 1: webOS 25, chipset o24n

| | |
|---|---|
| Date | 2026-11-11 |
| Lab device | `webOS25-o24n-dvb-2`, United Kingdom unit |
| Model name the TV reports | `KEY0000002E` |
| Platform | webOS 25 (2025), `webOS 10.2.0`, firmware `02.01.49` |
| Chipset | `o24n`, registered in Seller Lounge as `O24N(webOS 25)` |
| Memory, kernel | 1909 MiB, `aarch64` (the app is 32-bit ARM) |
| Panel, app surface | 3840x2160, 1920x1080 at pixel ratio 1 |
| Locale | `en-GB` |
| Build | `example/`, release, packaged by `rn-webos package` at `2b31798`, one entry per run |
| Installed to | `/media/developer/apps/usr/palm/applications/org.rnwebos.example/` |

| Area | What ran | Seen |
|---|---|---|
| Start | The executable from the unsigned `.ipk`, type `native` | Starts, opens its window and draws the home screen. `Platform.OS` is `webos`, `Platform.Version` `10.02` |
| Network | `fetch` to `https://api.github.com/zen`, `expo/fetch` text, JSON, body stream and abort | All answer: the compiled-in libcurl, Mbed TLS and CA bundle work on the TV |
| Remote keys | Down, Left, Right, OK, Back, the digits 1 to 6 | Arrive as `down`, `left`, `right`, `select`, `back` and `1` to `6` |
| Long press | Enter held 1.2 s, Right held 1.2 s, through the lab's keyboard mode (Linux key codes 28 and 106) | `longSelect`, `longRight`. A short Enter is `select` |
| Back | At the app's root | The TV's home screen shows, the process stays, and a launch brings the app back with its state |
| Back in a Modal | `onRequestClose` | Fires once, the app stays |
| Home | Home, then a launch from the lab | The app comes back with its state and logs `relaunch {}`. No `AppState` change was logged in between |
| Pointer | The lab's cursor over a `Pressable` row, then a press | The row under the cursor is pressed |
| Wheel | Five notches over a `ScrollView` | The content offset goes to 500: 100 points a notch |
| TextInput | A field with `autoFocus` | LG's on-screen keyboard opens. Typing "kroma" gives `key`, `change` and `selection` events in order; Enter gives `submit`, `blur` and `endEditing` and closes the keyboard |
| Views | Boxes (radii, borders, shadows, outlines, opacity), `Image` (network JPEG, bundled PNG, `data:` URI, a 404), transforms with perspective, text (truncation, alignment, decorations, case), `ScrollView`, `Modal`, `Alert` | Drawn as in the simulator |
| Filters | `expo-blur` over a still and a moving backdrop, masked views with text, SVG and gradient masks | Drawn as in the simulator |
| react-native-svg | The SVG screen: icons, shapes, clip paths, masks, patterns, text | Drawn as in the simulator |
| Gradients | Linear and radial backgrounds, dashed and dotted borders | Drawn as in the simulator |
| Native animated driver | 12 views on `translateX` and `opacity`, `useNativeDriver: true` | Animates |
| Expo | The five check screens: runtime, device and constants, file system, fonts, `expo-image` and `expo-blur` | Every check passes. `Paths.document` is under the install directory above |
| Intl | The French and English screen: dates, relative times, numbers, currency, plurals, collation | Formats |
| Video, HLS with MPEG-TS segments | `test-streams.mux.dev/x36xhzz/x36xhzz.m3u8` | Plays in the box, the picture showing through the app's window. Starts at 320x184 and reaches 1920x1080 at 60 fps |
| Video, HLS with fMP4 segments | Apple's `img_bipbop_adv_example_fmp4` | Plays, 480x270 `avc1` |
| Video, Matroska | `test5.mkv`: H.264, two audio tracks, eight subtitle tracks | Plays. Both audio tracks and the eight subtitle tracks are listed, the next audio and subtitle track can be chosen, and the subtitle is drawn |
| Video, fullscreen | `enterFullscreen()` | Fills the screen, subtitles included |
| Video, an HTTP error | Two files on `repo.jellyfin.org` | The TV's player reports `streaming protocol error (40403)`, and the player shows status `error`. The same URLs answer 206 from another network, so the host refuses the lab |
| Memory | The bench screen's own `VmRSS` label | 64.8 MB with the 1,000-row list still; 86.1 MB, peak 98.0 MB, with focus moving one row every 50 ms, which the list keeps up with |

Not run on this TV: pause and seek, HEVC, HDR and DRM video (the HEVC and HDR
samples are on the host that refused the lab), the modules screen (it needs an
echo server on the developer's network), a deep link with parameters, voice,
and every `rn-webos` command that talks to a TV.

Seen once and not explained: on the first launch, with the lab's cursor still
on screen, the first Right press arrived as `left`. No later launch or key
press repeated it.

## TV 2: webOS 24, chipset k24

| | |
|---|---|
| Date | 2026-11-11 |
| Lab device | `webOS24-K24-DVB-1`, United Kingdom unit |
| Model name the TV reports | `KEY0000313F` |
| Platform | webOS 24 (2024) |
| Chipset | `k24`, registered in Seller Lounge as `K24(webOS 24)` |
| Build | KROMA's TV app, an Expo app written for tvOS, Android TV and Tizen, packaged by `rn-webos package` from a trial branch: release, 14.4 MB |

| Area | What ran | Seen |
|---|---|---|
| Start | The app's own start: an intro video from a bundled file through `expo-video`, then its first screen | Starts, plays the intro, draws |
| Remote keys | The app's JavaScript focus navigator, fed by `useTVEventHandler` | Arrows move the focus, OK selects, across its profile, server, home, player and settings screens |
| Back | `BackHandler` listeners | `hardwareBackPress` reaches them and the app goes back a screen. Before the app listed `webos` among the platforms that send Back this way, nothing consumed it and the TV went to its home screen |
| Long press | Enter held 1.3 s on a list item | The app's `longSelect` handler opens its item menu |
| Network | HTTPS to the app's own server, its sign-in by code and its catalog with artwork | All load |
| Video | A film the app labels H.265, 4K, HDR, from that server, in the app's player | Plays, with the app's controls and seek thumbnails over it. Whether the server transcoded it is not known |
| Pointer | The lab's cursor on the app's controls, once the app drew pressable faces on webOS | A press activates the control under the cursor |
| App data | The lab re-installing the package | The app's stored sign-in was gone afterwards: what it keeps under `Paths.document` did not survive |

## What a TV still has to settle

- Frame rate and start time, which need `rn-webos perf` or the bench reports,
  so a TV in Developer Mode or a way for a lab TV to reach the computer.
- A native crash: the report and its symbolization.
- Chipsets other than `o24n` and `k24`, 1280x720 TVs and webOS before 24.
- Where an app's data must live to survive an update: on TV 2 it was gone after
  a re-install by the lab, which may remove the app first.
- `connect`, `install`, `run`, `key`, `logs`, `perf` and `screenshot`.
