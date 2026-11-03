# TV app integration: picture-in-picture, background playback, voice

What a .NET TPK carrying a native `.so` may do on a Samsung TV for three
features a media app asks for, and how react-native-tizen would expose each to
an Expo app. Checked on 2026-11-03.

| Tag | Meaning |
|---|---|
| **D** | Documented: a Samsung or Tizen page, a Tizen Studio header or privilege list |
| **I** | Inferred from the rows it follows; not stated anywhere |
| **H** | Verified on hardware: the GQ75LS03DAUXZG (The Frame, 2024, Tizen 9.0), Developer mode, a TPK signed with a Public-level author certificate, 2026-11-03 |
| **U** | Unknown |

Certificates, partner groups and the manifest are in
[tizen-tv-platform.md](tizen-tv-platform.md) (sections 1.2, 1.3, 3); the
player, the overlay plane and its interruptions in
[tizen-media-playback.md](tizen-media-playback.md); the remote's voice key in
[tizen-input.md](tizen-input.md) (section 9). A store build already needs a
Seller Office partner group for the native `.so` itself, so "partner" below
means a requirement on top of that one.

## Verdicts

| Feature | Public certificate (Developer mode) | Needs Samsung partner status or a deal | Impossible |
|---|---|---|---|
| Picture-in-picture over other apps | An in-app mini player (the video in a small view inside the app's own screen) | Being listed in Multi View, which Samsung curates per app and region | The app's own video in a window over the home screen or another app: no public or partner API exists |
| Playback while hidden or closed | `background-category` `media` may keep audio running while hidden: undocumented on TV, to be tried | Nothing documented; Samsung's rules say a hidden app pauses | Video while hidden (the window is hidden); anything after the app is closed |
| Voice | Play, pause, seek, skip, subtitles and in-app search through Samsung's voice interaction, privileges public | Search from the home screen ("play Blade Runner") into the app's catalog | Indexing a self-hosted library in Samsung's search |

## 1. Picture-in-picture

### What Samsung documents

| Fact | Tag | Source |
|---|---|---|
| "Can I create an overlay application that runs on top of and interacts with another application?" "No. Overlay applications are not supported on Samsung TVs." | D | [faq] Q10 |
| "The Ticker feature, which allowed an application to run as an overlay, is no longer supported." | D | [faq] Q16 |
| Samsung's PiP is the reverse of Android's: it shows "TV source video, such as a TV channel or HDMI input, through a cutout in your application screen", with the TVWindow API | D | [faq] Q11, [pip] |
| `tizen.tvwindow`: `show(success, error, rectangle, type, zPosition)`, `hide`, `setSource`, `getSource`, `getRect`, `getAvailableWindows`, `getVideoResolution`, resolution listeners. `WindowType` has one value, `MAIN`. `ZPosition` `FRONT` (default) or `BEHIND` since 2.4. Privilege `http://tizen.org/privilege/tv.window`, public; feature `http://tizen.org/feature/tv.pip` | D | [tvwin] |
| "All Samsung TVs since 2015 support PiP"; overlays on a PiP window since 2016; volume through `tizen.tvaudiocontrol`, privilege `http://tizen.org/privilege/tv.audio` (public) | D | [pip], [pip-ov], [cfg] |
| Every TVWindow page is a web API (`config.xml`, JavaScript); TizenFX TV's `Tizen.TV.Application.Window` namespace holds two enums, `ActionType` and `ButtonType` | D | [tvwin], [tvfx-win] |
| Multi View is a user feature: Settings, then Multi View; "Only certain 2023 Samsung TVs support the Multi View feature", 2 to 4 screens by model; a "Set Picture-in-Picture" layout; apps listed are Samsung TV Plus, YouTube, YouTube TV, Melon, Barre3 and web browsers, and "The list of supported apps may differ by region and may be updated without notice" | D | [mv] |
| No developer page documents Multi View, a mini player, or a floating window | D (absence) | [tvwin], [pip], [mt], [tvfx] |

### What an app can use

| Path | Status | Certificate |
|---|---|---|
| In-app mini player: a small `VideoView` in the app's own layout; the overlay plane follows its rect | Works today ([tizen-media-playback.md](tizen-media-playback.md), section 3, H on the test TV) | Public |
| TVWindow | Shows the tuner or HDMI, not the app's media; web apps only (D, [tvwin]) | Public, web only |
| A second window at notification level (`efl_util_set_notification_window_level`, `http://tizen.org/privilege/window.priority.set`, public, "for a notification type window only") with a player bound to it | U, unlikely: a player's display cannot move to another window once set ([tizen-media-playback.md](tizen-media-playback.md), section 3), and Samsung forbids overlay apps ([faq] Q10) | Public, store-hostile |

### Design

Map expo-video's PiP surface and change nothing about it:

| expo-video | On Tizen |
|---|---|
| config plugin `supportsPictureInPicture` | ignored; `rn-tizen package` prints one line saying Tizen has no picture-in-picture |
| `isPictureInPictureSupported()` | `false`, as today |
| `allowsPictureInPicture`, `startsPictureInPictureAutomatically` | accepted, no effect |
| `startPictureInPicture()` | rejects with "unsupported on Tizen", as today |

The in-app mini player needs no API: the app renders a small `VideoView`.
Multi View gets no key: no page documents how an app joins it, and Samsung
picks its apps (D, [mv]).

## 2. Playback while hidden or closed

### What is documented

| Fact | Tag | Source |
|---|---|---|
| "Since 2015 TV models, applications are automatically paused when they are hidden. For compatibility reasons, in the "config.xml" file, the `background-support` attribute must be set to "disable". Do not modify it." | D | [faq] Q22 |
| "When the user switches from your application to another application or TV channel, JavaScript execution is paused"; "When the application is hidden during media playback, implement the same behavior as clicking the "Return" key during playback" | D | [mt] |
| CO-MT-01: during playback, Smart Hub sends the app to the background; on resume "media playback resumes in the same state as before" | D | [common] |
| Tizen: "An application is not allowed to run in the background except when it is explicitly declared to do so." `background-category` `media` covers "Playing audio, recording, and outputting streaming video in the background"; the others are `download`, `background-network`, `location`, `sensor`, `iot-communication`. Tizen 4.0+ for .NET, 2.4+ native; older API levels use `<metadata key="http://tizen.org/metadata/background-category/media"/>` | D | [td-uiapp], [td-overview] |
| "Since Tizen 2.4, an application in the background goes into a suspended state ... executed with limited CPU resources", except "Media and Download" style apps that declare a category | D | [td-overview], [td-lifecycle] |
| No Tizen or Samsung page says whether TV honours `background-category` | D (absence) | [td-uiapp], [mt], [faq] |
| `player_interrupted_cb` with `PLAYER_INTERRUPTED_BY_RESOURCE_CONFLICT` leaves the player in IDLE, READY or PAUSED; a second handle preparing interrupted the first on the test TV | D, H | [tizen-media-playback.md](tizen-media-playback.md), sections 4 and 5 |

### What happens to the player

| Moment | Video | Audio | Tag |
|---|---|---|---|
| App hidden (Home, another app, Instant On power-off), no category | gone with the window | suspended with the process | I from [td-overview] and [faq] Q22 |
| App hidden, `background-category` `media` | gone with the window | may continue; whether the overlay player keeps decoding with its window iconified is unknown | U |
| App hidden by launching another app, `background-category` `media`, `staysActiveInBackground` `false` | | the host pauses the player at `OnPause`; JavaScript timers keep running while hidden; back in front, the player stays paused | H |
| The same, `staysActiveInBackground` `true` | | the player keeps decoding: a 30 s clip reached its end and looped 25 s after the app was hidden, while JavaScript kept logging every 5 s. Whether sound is heard was not checked (the clip has no audio track) | H |
| A visible app asks for the decoder | | interrupted (`PLAYER_INTERRUPTED_BY_RESOURCE_CONFLICT`), the player stays PAUSED or READY | D for the callback; H for a second handle |
| App closed (Return on the root screen, Exit, a low-memory kill) | the player ends with the process | ends | I |
| A service app playing sound | it has no window | unknown | U |

So nothing continues after the app is closed, and nothing shows while it is
hidden. Audio while hidden is an experiment for Developer mode, and a store
build doing it would contradict [faq] Q22 and [mt]. I

### What react-native-tizen does

Built as the table below says ([`../tv-features.md`](../tv-features.md#background-audio)):
`src/config/tv-features.ts` reads the plugin option, `src/package/feature-manifest.ts`
writes the category, and `pauseMediaSessionsForBackground` in
`runtime/host/src/tv/MediaSessions.cpp` pauses at `rn_tizen_host_pause`.

expo-video already names both halves: the config plugin's
`supportsBackgroundPlayback` and the player's `staysActiveInBackground`
("Determines whether the player should continue playing after the app enters
the background", default `false`; [ex] `VideoPlayer.types.ts`).

| Piece | Where | Does |
|---|---|---|
| `["expo-video", { "supportsBackgroundPlayback": true }]` in `plugins` | `rn-tizen package` | adds `<background-category value="media" />` to the `ui-application`. No Tizen key: the same `app.json` works on Android and iOS. a release build prints that Samsung pauses hidden apps ([faq] Q22) |
| `OnPause` | shell | already calls `rn_tizen_host_pause` |
| Pause on background | host | every `VideoPlayer` whose `staysActiveInBackground` is `false` pauses at `rn_tizen_host_pause`, as expo-video's Android `VideoManager.onAppBackgrounded` does ([ex] `android/.../managers/VideoManager.kt:154-191`); nothing resumes by itself at `OnResume`, and `AppState` turns `active` as today |
| `staysActiveInBackground: true` | host | the player keeps running; without the manifest category the host logs that the TV will suspend the app |
| Interrupted while hidden | host | `playingChange` to `false` and the player kept; `play()` on return re-acquires the decoder ([tizen-media-playback.md](tizen-media-playback.md), section 5) |

`showNowPlayingNotification` and `audioMixingMode` stay without effect: no
Tizen TV surface shows a now-playing card to a third-party app (D by absence,
[mt], [tvfx]).

## 3. Voice

### What "play a film with voice" can mean

| Reading | Where it is answered |
|---|---|
| Voice commands: "play Blade Runner", "pause", "skip forward 30 seconds", "subtitles on" | this section |
| The film's audio tracks: languages, commentary | done: `audioTrack`, `availableAudioTracks` ([tizen-media-playback.md](tizen-media-playback.md), section 7) |
| Audio output: a soundbar, Dolby, passthrough | no app API; the TV's "Digital Output Audio Format" setting; DTS, TrueHD, FLAC need the server's transcode ([tizen-media-playback.md](tizen-media-playback.md), section 2) |
| Sound that keeps playing after leaving the app | section 2 |
| Spoken menus for blind users (Voice Guide) | out of scope here; the web metadata `use.voiceguide` appears in [cfg] |

### What Samsung documents

| Fact | Tag | Source |
|---|---|---|
| `webapis.voiceinteraction`: "Samsung TVs allow developers to use voice commands such as Navigation, Search, Selection and Media Control"; "The Voice Assistant in the TV can be Bixby or other assistants". Since 6.0, product TV, privilege `http://developer.samsung.com/privilege/voicecontrol`, level Public, `required_version` 6.0 | D | [vi] |
| Methods: `getVersion`, `setCallback(callback)`, `listen()`, `buildVoiceInteractionContentContextItem(x, y, title, aliases, focused)`, `buildVoiceInteractionContentContextResponse(items)` | D | [vi] |
| Callbacks since 6.0: `onupdatestate` (returns `None`, `Home`, `List`, `Player`, `Setting`, `Search` or `Unknown`, "called right before processing every utterance"), `onnavigation`, `onselection` (relative index: -1 last, 0 current, 1 first), `onplay`, `onpause`, `onfastforward`, `onrewind`, `onchangeappstate`, `onchangeprevioustrack`, `onchangenexttrack`, `onrestart`, `onskipbackward(seconds)`, `onskipforward(seconds)`, `onsetplayposition(seconds, absolute)`, `onchangesubtitlemode`, `onchangeshufflemode`, `onchangescreenfitmode`, `onzoom`, `onrotate`, `onchange360mode`, `onchangerepeatmode` | D | [vi] |
| Since 6.5: `ontitleselection(title)` for "Select {Title Name}", which needs `onrequestcontentcontext` to return the titles on screen with position and aliases; `onadditiontolist`, `onremovalfromlist`. `onplaylist`, `onbrowselist`, `onskipad` are "only available for 25Y TV models at the moment" | D | [vi] |
| "If a callback returns the false/undefined value, or there is no callback implementation for the utterance, Samsung device may perform its basic function": for `onplay` it generates the "MediaPlay" key, for `onpause` "MediaPlayPause", for navigation the arrow keys | D | [vi] |
| The page defines no search callback, no launch path for an app that is not running, and says nothing about foreground, .NET or native apps | D (absence) | [vi] |
| An older FAQ answer: "Voice control is supported only for TV features ... It is not supported within applications." VoiceInteraction (6.0) postdates it | D, I | [faq] Q4 |
| The Samsung Product API list names VoiceInteraction (6.0) and Microphone (2.3), and no Bixby API | D | [papi] |
| Tizen's native voice control (`voice_control.h`, `vc_initialize`, `VC_COMMAND_TYPE_FOREGROUND`) needs `http://tizen.org/privilege/recorder`, public, and an installed engine | D | rootstrap `voice_control.h:57-100` |
| TizenFX has `MediaControlServer` (privilege `http://tizen.org/privilege/mediacontroller.server`, public; play, pause, seek and search commands from a client) | D | [tfx-mc], SDK `privilege-dotnet.properties:36` |

### What the TV does

| Fact | Tag |
|---|---|
| The TV's framework carries `Tizen.TV.Service.VoiceInteraction` 1.0.8.0, found by `Type.GetType` from the shell | H |
| `PrivilegeChecker.CheckVoiceInteractionPrivilege()`, called by reflection from a TPK holding `voicecontrol` and `microphone`, returns without throwing: the class would accept the app | H |
| `MessagePort.Send` to `com.samsung.tv.vif`, port `com.port.vif.voiceinteraction`, succeeds, so the voice service and its port exist; the `register` message is sent | H |
| Which thread the message port's callbacks run on, and what each utterance sends | U: needs someone speaking (see Verify, step 10) |

### The protocol the shell speaks

The shell speaks Samsung's voice service protocol directly with public TizenFX
APIs (`MessagePort`, `Bundle`, `EventReceiver`), which keeps Tizen.NUI out of
the process. Only the registration is seen on the TV so far; the rest stays U
until Verify, step 10.

| Part | What `shell/VoiceChannel.cs` does | Tag |
|---|---|---|
| Registration | listens on a local port `com.port.vif.voiceinteraction` and sends `com.samsung.tv.vif`, on the same port name, a bundle `{command, strData, totalDataLength, sdkVersion, description}`: `register` with the app id as `strData` at start, `unregister` at exit | H for the send |
| Restart | registers again on the event `event.com.samsung.tv.vif.InitEvent` | U |
| Commands | a bundle whose `command` is an `ACTION_*` name and whose `strData` is its argument: seconds as an integer string, `on` or `off` for subtitles, an ordinal for a selection (-1 last, 0 this one, 1 first), the title as text, and for a search a bundle with `utterance`, `title`, `genre`, `cast`, `content_type`, `from`, `to`, `raw` | U |
| Answers | `appContext` with the state name (`None`, `Home`, `List`, `Player`, `Setting`, `Search`, `Details`, `Unknown`) to `ACTION_REQUEST_APP_CONTEXT`; `result` with the content context to `ACTION_REQUEST_CONTENT_CONTEXT`; `result` with `success` or `not_support` to any other action | U |
| Content context | a JSON array of `{"positionX", "positionY", "title", "alias", "bFocused"}`; the web page gives the positions no unit and its example passes `(1, 1)` | U; D [vi] for the unit |

### How "play Blade Runner" reaches an app

| Where the user is | Route | For KROMA |
|---|---|---|
| Inside the app | Bixby, then the voice service, then a search command with `title` = "Blade Runner" (U, Verify step 10); the web API has no search callback (D) | possible: the app searches its own server and plays |
| Inside the app, the title on screen | `onrequestcontentcontext` returns the visible titles, `ontitleselection("Blade Runner")` picks one (6.5+) | possible |
| Home screen or another app | Samsung's search app and Samsung's server catalog (U) | no documented route for a self-hosted library: [vi] defines no launch path for an app that is not running (D); "open KROMA" by voice is a consumer feature with no developer page (U) |
| Playing | `onplay`, `onpause`, `onskipforward(30)`, `onsetplayposition(4800)`, `onchangesubtitlemode` in the web API's names | possible; play, pause, fast forward and rewind may also arrive as remote keys when the app does not answer (D, [vi]) |

Versions: the callbacks need Tizen 6.0 (2021 sets), title selection 6.5
(2022) ([vi]); the test TV, a 2024 set on Tizen 9.0, should have both and lacks
the "25Y" ones (I). Its framework carries the .NET assembly (H, above).

### What react-native-tizen does

Built; the developer's guide is [`../tv-features.md`](../tv-features.md#voice).

| Piece | Where | Does |
|---|---|---|
| `expo.tizen.voice` (`true`, or `{ "searchUrl": "myapp://search?q={query}" }`) | `src/config/tv-features.ts` | adds `voicecontrol` and `microphone` and writes `voice` into the runtime config |
| `VoiceChannel` | `shell/VoiceChannel.cs` | speaks Samsung's voice service protocol directly with public TizenFX APIs, which keeps Tizen.NUI out of the process; answers only messages from `com.samsung.tv.vif`; probes the class by reflection at startup and logs whether it would accept the app |
| `VoiceCommands` | `shell/VoiceCommands.cs` | turns `ACTION_*` and `strData` into `{"type": ...}` JSON for the host |
| `rn_tizen_host_voice_state`, `_context`, `_command` | `runtime/host/src/CApi.cpp`, `src/tv/VoiceController.cpp` | answered on the main thread: the state from `Voice.setScreen`, else `Player` while a video plays, else `List`; the content context from the mounted views; the command |
| Transport | `src/tv/MediaSessions.cpp`, `expo/modules/video/VideoPlayerSession.cpp` | to JavaScript's `command` listeners, else to the playing expo-video player (play, pause, stop as pause, restart, skip, seek, subtitles on or off); `next`, `previous`, `fastForward`, `rewind` stay with the TV, which sends keys |
| Selection | `src/tv/ContentContext.cpp` | every visible view with an `accessibilityLabel`, in reading order; positions are the views' centers in points of the 1920x1080 screen (I: the unit is undocumented). "select Dune" matches the label ignoring case and punctuation, then by prefix, then by substring, and sends a touch start and end to that view, which runs a `Pressable`'s `onPress` (seen in the simulator) |
| Search | `src/tv/VoiceController.cpp`, `src/tv/SearchUrl.cpp` | to the `search` listeners, else `searchUrl` filled and opened as a `url` event |
| `@kromatv/react-native-tizen/voice` | `src/api/voice.ts` | `Voice.isAvailable`, `setScreen`, `addListener('search' \| 'command')` over the `RNTizenVoice` TurboModule and `DeviceEventEmitter`; payloads parsed with zod; nothing on other platforms |

## Verify on a TV

On the 2024 Samsung The Frame (LS03D), Tizen 9.0, in Developer mode. Record each
answer here as H with the model, year and Tizen version.

Picture-in-picture:

1. Settings: does the set have Multi View, and which apps does it offer?
2. Is any app installed in Developer mode offered in Multi View, and does its
   player play in a zone?
3. A second notification-level window with `window.priority.set`: after
   Home, is it still visible, did `OnPause` fire, does a player bound to it
   play?

Background:

4. No category: play, press Home, then log for 30 s from a timer: `OnPause`,
   the player state, whether the position advances, whether sound is heard.
5. The same with `<background-category value="media" />`. Answered for a
   hidden app (another app launched over it rather than Home): section 2, H.
   Sound and the no-category case are still open.
6. While hidden, play a video in another app: the interrupted code, then
   return and log the state and whether `player_start` resumes.
7. Return on the root screen, then Exit: the process is gone and nothing plays.

Voice:

8. No voice code: hold the mic and say "pause", "play", "fast forward": which
   key names reach the app?
9. From the shell, `Type.GetType("Tizen.TV.Service.VoiceInteraction.VoiceInteractionHandler, Tizen.TV.Service.VoiceInteraction")`:
   present, and which version? Answered: 1.0.8.0, and its privilege check
   passes (section 3, H).
10. With both privileges and a handler: "search Blade Runner", "skip forward
    30 seconds", "go to one hour twenty", "select <title on screen>",
    "subtitles on": which hooks fire, on which thread, with which search fields.
    Run the example's platform screens (`src/screens/platform/main.ts`) and
    read the `Voice:` log lines with `--level debug`: the shell logs each
    command and the thread once at `info`, and the command's data at `debug`.
11. Without the privileges: does the class's privilege check throw?
12. From the home screen: "open KROMA", "play Blade Runner on KROMA".
13. Does `XF86BTVoice` still reach the app while the assistant listens?

## Sources

Local: the Tizen 9.0 rootstrap headers
(`voice_control.h`, `ui/efl_util.h`) and SDK privilege lists under
`~/tizen-studio/platforms/tizen-9.0`. expo-video 58.0.12 in `node_modules`
([ex]: `src/VideoPlayer.types.ts`, `src/VideoView.types.ts`,
`plugin/src/withExpoVideo.ts`).

[faq]: https://developer.samsung.com/smarttv/develop/faq/other-features.html
[mt]: https://developer.samsung.com/smarttv/develop/guides/fundamentals/multitasking.html
[common]: https://developer.samsung.com/smarttv/develop/development-checklist/common.html
[cfg]: https://developer.samsung.com/smarttv/develop/guides/fundamentals/configuring-tv-applications.html
[tvwin]: https://developer.samsung.com/smarttv/develop/api-references/tizen-web-device-api-references/tvwindow-api.html
[pip]: https://developer.samsung.com/smarttv/develop/guides/pip-picture-in-picture/managing-pip-windows.html
[pip-ov]: https://developer.samsung.com/smarttv/develop/guides/pip-picture-in-picture/showing-pip-overlays.html
[vi]: https://developer.samsung.com/smarttv/develop/api-references/samsung-product-api-references/voiceinteraction-api.html
[papi]: https://developer.samsung.com/smarttv/develop/api-references/samsung-product-api-references.html
[tvfx]: https://developer.samsung.com/smarttv/develop/api-references/tizenfx-tv-api-references/tizen.tv.html
[tvfx-win]: https://developer.samsung.com/smarttv/develop/api-references/tizenfx-tv-api-references/tizen.tv.application.window.html
[mv]: https://www.samsung.com/uk/support/tv-audio-video/use-the-multi-view-feature-on-your-samsung-smart-tv/
[td-uiapp]: https://github.com/Samsung/tizen-docs/blob/master/docs/application/dotnet/guides/applications/uiapplication/ui-app.md
[td-overview]: https://github.com/Samsung/tizen-docs/blob/master/docs/application/native/guides/applications/overview.md
[td-lifecycle]: https://github.com/Samsung/tizen-docs/blob/master/docs/application/dotnet/guides/applications/uiapplication/application-lifecycle.md
[tfx-mc]: https://github.com/Samsung/TizenFX/blob/main/src/Tizen.Multimedia.Remoting/MediaController/MediaControlServer.cs
[ex]: https://github.com/expo/expo/tree/main/packages/expo-video
