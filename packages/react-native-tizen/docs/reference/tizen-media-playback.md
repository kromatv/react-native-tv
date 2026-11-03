# Tizen media playback

What a Samsung TV gives a native app for video, and how `expo-video` maps onto
it. Read before touching anything that plays media.

Every fact carries a source tag (table at the end) and how it is known:
**D** documented, **I** inferred (the reason follows), **U** unknown,
**H** seen on hardware (which set follows; **H-1** is the set in the sources
table). `PH:1042` means line 1042 of the Tizen 9.0 rootstrap's `media/player.h`.

## 1. Which player a TV app can call

| API | What it is | Privilege, level | Usable here | Known |
|-----|-----------|------------------|-------------|-------|
| `player_*` (capi-media-player) | Tizen's public C player, `libcapi-media-player.so.0`, header in every rootstrap | `internet` for URLs, `mediastorage` / `externalstorage` for files, public | Yes, from `libreact-native-tizen.so` directly | D [PH:606-632] |
| TV exports of the same `.so` that no public header declares: `player_set_ecore_wl_display`, `player_set_drm_handle`, `player_set_drm_init_complete_cb`, `player_set_drm_init_data_cb`, `player_get_adaptive_streaming_info`, `player_get_track_count_v2`, `player_get_{video,audio,subtitle}_track_info_v2` | Window-rect display, DRM, live info, codec-level track info | none beyond the above; `drmplay` for DRM | Through `dlsym`, as optional features | D that they exist [TFX-I, FH-old]; their ABI is private and can change per release, I from flutter-tizen moving them into a prebuilt wrapper per release [FH, FA] |
| `Tizen.Multimedia.Player` (TizenFX) | C# wrapper over the same C API | same | Only through the .NET shell; nothing gained | D [TFX-X] |
| `Tizen.TV.Multimedia.Player` | Samsung TV subclass: adds MPEG-DASH, Smooth Streaming, PlayReady through `DRMManager`, `SetFixedMaxResolution` | `drmplay` for DRM | .NET only | D [SN, STP] |
| `Tizen.TV.Multimedia.ESPlayer` | Elementary-stream player (the app demuxes and feeds packets) | `esplay`, **Partner** | No: partner certificate | D [STN] |
| plusplayer (`video_player_avplay`) | Samsung's TV player libraries, shipped prebuilt by flutter-tizen | `drmplay` for DRM | No: prebuilt binaries, one build per Tizen version | D [FA] |
| `webapis.avplay` | The web app player KROMA's Tizen web client uses | none since 2015 models | No: web apps only | D [SP] |

On `Tizen.TV.Multimedia.Player` Samsung lists these limits: `CaptureVideoAsync`,
`ApplyAudioStreamPolicy`, `AudioEffect` and `VideoFrameDecoded` throw
`NotSupportedException`; any playback rate other than 1.0 mutes the sound;
RTP/RTSP is not supported; DRM does not play on the TV emulator. D [SN]

Which C calls the TV subclass makes underneath is U. The decoders, demuxers and
streaming engines it reaches are I to be the same ones capi-media-player
reaches, because both subclass and base share one `Player` handle type [TFX-X].

## 2. Formats per model year

Model year to platform, from [SG]. .NET apps run from Tizen 4.0 (2018), so
older years cannot run this package at all. D [SG]

| Year | Tizen | TizenFX | HLS version | Widevine L1 | AV1 | DTS audio |
|------|-------|---------|-------------|-------------|-----|-----------|
| 2018 | 4.0 | 4 | v7 | 3.2 | no | not supported |
| 2019 | 5.0 | 5 | v7 | 14.1 | no | not supported |
| 2020 | 5.5 | 6 | v7 | 16.3 | UHD groups only, not 20TV_BASIC2 (FHD) | not supported |
| 2021 | 6.0 | 8 | v7 | 16.3 | every group | not supported |
| 2022 | 6.5 | 9 | v7 | 16.4 | every group | not supported |
| 2023 | 7.0 | 10 | v7 | 17.1 | every group | not supported |
| 2024 | 8.0 | 11 | v10 | 17.1 | every group | not supported |
| 2025 | 9.0 | 12 | v10 | 18.1 | every group | not supported |
| 2026 | 10.0 | 13 | v10 | 18.5 | every group | not supported |

All D: platform, HLS and Widevine from [SG], AV1 and DTS from each year's
[SV-year]. From 2023 a set can take an OS upgrade, so the year fixes the
minimum Tizen, not the exact one. D [SG]

| Topic | Fact | Known |
|-------|------|-------|
| Containers (file playback) | `.avi .mkv .asf .wmv .mp4 .mov .3gp .vro .mpg .mpeg .ts .tp .trp .flv .vob .svi .m2ts .mts`, plus `.webm`; `.rmvb` until 2025 | D [SV-2024, SV-2026] |
| HEVC | Main and Main10 every year listed (2016 adds Main 4:2:2 10). UHD Level 5.1, Level 5.2 on newer groups, 8K Level 6.1 on 8K groups from 2019 | D [SV-2016..2026] |
| HEVC containers | MKV, MP4 and TS only | D [SV-2017..2026] |
| H.264 | BP/MP/HP to 4096x2160 at 60 fps; no 10-bit listed | D [SV-2024] |
| VP9 | Profile 0 and 2, in WebM | D [SV-2018..2026] |
| Dropped codecs | WMV, VC-1, WMA gone on 24TV_BASIC4 and 2025+; RealVideo, RealAudio, RMVB gone on 2026+ | D [SV-2025] |
| Audio decoders (2024) | AC3, LPCM, ADPCM, AAC, HE-AAC, WMA, DD+ (5.1), MP3, MPEG-H (South Korea only), AC-4, Opus, G.711, Vorbis | D [SV-2024] |
| Not in any 2018+ list | DTS (stated), TrueHD, FLAC | D for DTS, I for the others from their absence [SV-2018..2026] |
| Audio over HLS fMP4 | AAC, AC-3, E-AC-3, MP3 prepare and play; FLAC, DTS, TrueHD fail `prepareAsync`; Opus prepares with zero channels | H (KROMA, AVPlay, a Tizen 9.0 set, model not recorded) [K-caps] |
| Streaming engines | MPEG-DASH (`.mpd`), Smooth Streaming (`.ism/Manifest`), HLS (`.m3u8`), picked by extension in AVPlay | D for AVPlay [SA]; U whether capi-media-player selects DASH by extension too |
| HLS details | fMP4 segments since Tizen 3.0; `#EXT-X-PLAYLIST-TYPE` listed as not supported; AES-128 on every year | D [SG] |
| HDR | HLG, HDR10 and HDR10+ listed as supported for 8K streaming | D [SA] |
| Dolby Vision | Not on Samsung TVs; Samsung backs HDR10+ instead | D by press [TR]; no Samsung spec page lists it |
| HDR signalling to the app | `player.h` has no HDR or colour API | D by absence [PH] |
| 4K / 8K | App plane 1920x1080 on every model; video plane 3840x2160 (UHD) or 7680x4320 (8K) | D [SG] |
| Subtitles, capi | `player_set_subtitle_path` takes a local absolute path, SubRip, SAMI or MicroDVD/SubViewer only, in IDLE | D [PH:2164-2179] |
| Subtitles, AVPlay | SAMI, SubRip, SMPTE-TT, WebVTT out-of-band (recommended for HLS); TTML/DFXP for Smooth Streaming only; external files must be downloaded first | D [SG, SS] |
| Audio passthrough | No player API selects bitstream output. It is the user's "Digital Output Audio Format" setting (Auto or Pass-Through) | D for the setting [SAT]; U whether an app's E-AC-3 leaves the set as bitstream |

## 3. Where the video is drawn

### Display types

| `player_display_type_e` | Value | Since | What it does | Known |
|------------------------|-------|-------|--------------|-------|
| `PLAYER_DISPLAY_TYPE_OVERLAY` | 0 | 2.3 | Video on its own hardware plane under the app window | D [PH:124-129, NG] |
| `PLAYER_DISPLAY_TYPE_EVAS` | 3 | 4.0 | Frames into an Evas image object inside the UI | D [PH:126]; deprecated since Tizen 11.0 [NG] |
| `PLAYER_DISPLAY_TYPE_NONE` | 4 | 4.0 | No display; frames reach the app through `player_set_media_packet_video_frame_decoded_cb` | D [PH:127, PH:1091] |
| `PLAYER_DISPLAY_TYPE_OVERLAY_SYNC_UI` | 5 | 6.5 | Overlay whose position commits with the UI's Wayland subsurface | D [PH:113-122] |
| `OVERLAY2` | 6 | 11.0 | What flutter-tizen passes on Tizen 11 | D [FH: ftpw header] |

Rules that hold for every type, all D from `player.h`:

- `player_set_display` runs only in IDLE, before prepare, and only on the
  application's main thread; from another thread it returns
  `PLAYER_ERROR_INVALID_OPERATION`, and the `ecore_thread_main_loop_begin`
  escape can deadlock [PH:1038-1068].
- A handle's display cannot be changed once set ("We are not supporting
  changing display") [PH:1042].
- `player_set_display_mode`, `_rotation` and `_roi_area` return
  `PLAYER_ERROR_NOT_AVAILABLE` under `OVERLAY_SYNC_UI`; the app moves that
  surface with `ecore_wl2_subsurface_exported_surface_move/resize` and commits
  with `ecore_wl2_window_commit` [PH:113-122].

### Overlay on an Elementary window

| Fact | Known |
|------|-------|
| TizenFX's `Display(ElmSharp.Window)` calls `player_set_display(OVERLAY, <the elm_win Evas_Object>)`, the case this package is in | D [TFX-D9:120-128] |
| TizenFX's `Display(NUI.Window)` calls the unexported `player_set_ecore_wl_display(player, type, Ecore_Wl2_Window*, x, y, w, h)`, default 1920x1080 | D [TFX-I] |
| The UI window above the video must be transparent where the video shows: TizenFX requires `NUI.Window.BackgroundColor` to be `Transparent` | D [TFX-X: Display(NUI.Window)] |
| Placement: `player_set_display_mode(PLAYER_DISPLAY_MODE_DST_ROI)`, then `player_set_display_roi_area(x, y, w, h)`; the ROI only applies in DST_ROI, minimum 1x1, may be set before the mode | D [PH:1435-1456] |
| Other modes: `LETTER_BOX`, `ORIGIN_SIZE`, `FULL_SCREEN`, `CROPPED_FULL`, `ORIGIN_OR_LETTER` | D [PH:222-230] |
| Source crop: `player_set_video_roi_area(x, y, w, h)` in 0..1 ratios of the video size picks part of the frame and fits it to the display; OVERLAY only, since 5.0 | D [PH:2524-2567] |
| AVPlay's own rect space is always 1920x1080, whatever the app resolution | D [SP] |
| flutter-tizen's videohole player sets DST_ROI and drives the ROI from the widget's geometry | D [FH: media_player.cc:226-236, 474] |
| flutter-tizen paints the video's rect with transparent pixels (`BlendMode.src`) to see the plane | D [FH: hole.dart:62] |
| Its example app leaves `IsWindowTransparent` at its default `false`, so per-pixel alpha in the UI buffer is enough on a TV | I from [FE:75] and the example's `App.cs` |
| `elm_win_alpha_set(win, EINA_TRUE)` turns the canvas alpha channel on, "subject to the underlying system supporting it" | D [EW:2170-2186] |
| `EVAS_RENDER_COPY` makes an object's pixels replace what is below, but Evas warns not to assume it can poke a hole in a window | D [EV:2205-2225] |
| This package's window has an opaque black rectangle `(0, 0, 0, 255)` behind every view. `setHostWindowTransparent` turns window alpha on while a video view shows a player and keeps that rectangle, so only the views' holes are transparent (section 7) | D [RNT: HostWindow.cpp] |
| With window alpha on and the background cleared, every pixel the app leaves undrawn shows what is under the window: on H-1 that was Samsung's home screen with its app icons, behind KROMA's player | H, seen on screen |
| On H-1, `elm_win_alpha_set` returns in 0.1 to 0.2 ms turning alpha on and 0.2 to 8.4 ms turning it off. On Wayland the change makes the GL engine create a new output window (efl-rendering.md section 1), yet KROMA's first frames took as long with alpha on from the window's creation (536 ms) as right after the switch (544 and 597 ms): the switch adds no frame time anyone can measure, so the window keeps alpha off until a video shows | H, timers around the call and `rn-tizen perf`, KROMA's launch with its intro video, 2026-11-03 |
| A view's `EVAS_RENDER_COPY` hole inside a subtree an ancestor maps (a scale or rotate transform) cuts nothing: the map renders the subtree into a surface first, then blends it. So while an ancestor maps the video view, the hole is a top-level object at the view's on-screen rect, and the ROI follows that rect every frame. Seen on H-1 with KROMA's settings card, which scales the player stage to 1070x446 | H, `rn-tizen screenshot` and the ROI log |
| On H-1 the picture shows with window alpha alone, the `EVAS_RENDER_COPY` hole alone, or both, and not with neither. With an opaque page background behind the video view, only both together show it: window alpha alone shows the background, the hole alone shows nothing | H, checked on screen |
| `player_set_display(OVERLAY, GET_DISPLAY(elm_win))` called from `libreact-native-tizen.so`, on the main loop, in IDLE, returns `PLAYER_ERROR_NONE`, and so do `player_set_display_mode(DST_ROI)` and `player_set_display_visible` | H-1 |
| `player_set_display_roi_area` in window pixels returns `PLAYER_ERROR_NONE` before prepare and while playing, for every rect the example's view took (1160x656 at 724,204; 576x324 at 724,604; 1920x1080 at 0,0); `player_set_video_roi_area` returns it too for a `cover` crop | H-1 |
| Nothing drawn by the UI can sit under the video; clipping, transforms and rounded corners never reach the plane, only the hole above it | I: the plane is a separate hardware layer [NG, TFX-X] |
| Every ROI change goes to the hardware compositor, so a no-op resize is worth skipping | H (KROMA, AVPlay `setDisplayRect`) [K-plane] |

### The other two paths

| Path | Gains | Costs | Known |
|------|-------|-------|-------|
| `EVAS` image object | Composes like any view: clip, transform, z-order | Deprecated in 11.0; 4K/HDR throughput on a TV is U | D [NG]; U for performance |
| `NONE` + decoded frames | flutter's `video_player_tizen` maps each TBM surface to a GPU texture and keeps only the newest frame | Needs the `multimedia.raw_video` feature; `Tizen.TV.Multimedia.Player` refuses `VideoFrameDecoded`; HDR is I to be lost on an 8-bit graphics plane | D [FV:58-84, 731-760; TFX-X; SN] |

## 4. States, threads and controls

### State machine (D [PH:60-66, 483-904; NG])

| Call | From | To |
|------|------|----|
| `player_create` | NONE | IDLE |
| `player_set_uri`, `_set_display`, `_set_streaming_buffering_time`, `_set_video_stream_changed_cb` | IDLE | IDLE |
| `player_prepare_async` | IDLE | READY when `player_prepared_cb` fires; since 5.0 `player_unprepare` cancels it from IDLE |
| `player_start` | READY, PLAYING, PAUSED (since 5.0) | PLAYING |
| `player_pause` | PLAYING | PAUSED |
| `player_stop` | PLAYING, PAUSED | READY |
| `player_unprepare` | any but NONE | IDLE |
| `player_destroy` | any but NONE | NONE |

- `player_completed_cb` fires at the end of stream; flutter-tizen pauses there
  [FV: OnPlayCompleted].
- `player_interrupted_cb(PLAYER_INTERRUPTED_BY_RESOURCE_CONFLICT)` leaves the
  player in IDLE, READY or PAUSED; check the state and react [PH:107, 319; NG].
- Since 5.0 HTTP streaming can pause itself to buffer; `player_start` during
  that forces playback on and stops `player_buffering_cb` [PH:804].

### Threads

| Callback | Thread | Known |
|----------|--------|-------|
| `player_subtitle_updated_cb`, `player_video_stream_changed_cb`, decoded-frame callbacks, media-stream callbacks | Not the main loop | D [PH:1074, 1115, 1258, 1293, 2184, 2245] |
| `ErrorOccurred`, `VideoStreamChanged`, `SubtitleUpdated` in TizenFX | Internal thread | D [TFX-X] |
| prepared, completed, seek-completed, buffering, interrupted, error | Not stated | U in the header; flutter-tizen queues every event and replays it on its main context [FV:253-277] |
| prepared, buffering, seek-completed, completed, interrupted, subtitle-updated, video-stream-changed | Off the main loop, on a thread of the player's: every one of them, every time, for every source in section 7's list | H-1; the error callback did not fire in those runs |
| `player_set_display` | Must be the main thread | D [PH:1045] |
| `player_create`, `player_prepare`, `player_unprepare`, `player_destroy` | No thread named in the header. TizenFX calls `player_create` and `player_unprepare` on the caller's thread with no check, runs the parameterless `PrepareAsync` (the synchronous `player_prepare`) on a thread-pool thread (`Task.Factory.StartNew`, `LongRunning`), and calls `player_destroy` from its `SafeHandle.ReleaseHandle` | D by absence [PH:494-604]; D [TFX-P] |
| `player_create`, `player_prepare_async`, `player_unprepare`, `player_stop`, `player_destroy` and the callback unsets, called on one thread of this package's, off the main loop, with `player_set_display` and every other call on the main loop between them | Every call returns `PLAYER_ERROR_NONE`; every callback still arrives off the main loop on the player's own threads; the next handle prepares and plays. Seen for ten handles in turn (seven sources, a seek and a track change at READY, two handles at once with the resource-conflict interrupt as before), five `replace`s on one handle (an unprepare each), and KROMA's intro and player | H-1, `example/src/screens/video/probe.ts` and `main.ts`, KROMA, 2026-11-03 |

### Controls

| Control | Fact | Known |
|---------|------|-------|
| Seek | `player_set_play_position(ms, accurate, cb)` or `_nsec`, in READY, PLAYING or PAUSED. `accurate=false` lands on a keyframe. Wait for the callback before the next seek (`PLAYER_ERROR_SEEK_FAILED`); `player_start`/`_pause` before it returns `INVALID_OPERATION`; non-seekable content returns `INVALID_OPERATION` and keeps playing | D [PH:868-937] |
| Seek at READY | AVPlay blocks five seconds and refuses it; the same seek once PLAYING lands | H (KROMA, AVPlay, Tizen 9) [K-landing] |
| Seek at READY, capi | Accepted: `player_set_play_position` returns `PLAYER_ERROR_NONE` at READY, the seek-completed callback follows within a second, and `player_start` then plays from there. With `accurate = false` it lands on the keyframe at or before the target (29950 ms gave 24950 ms in an MP4; 20000 ms gave 20000 ms in an MKV) | H-1 |
| Audio track at READY | AVPlay refuses the change at READY and accepts it once PLAYING | H (KROMA, AVPlay) [K-direct] |
| Audio track at READY, capi | Accepted: `player_select_track(AUDIO, 1)` returns `PLAYER_ERROR_NONE` at READY, and track 1 is still the current one after `player_start` (Matroska test5) | H-1, as the header allows [PH:2367] |
| Position, duration | `player_get_play_position`, `player_get_duration` (ms or ns); no periodic time callback exists | D [PH:954-1689] |
| Buffering | `player_buffering_cb(percent)` for network sources only | D [PH:468, 2105] |
| Buffer size | `player_set_streaming_buffering_time(prebuffer_ms, rebuffer_ms)` in IDLE; values above 1000 ms, 0 platform default, -1 keep | D [PH:2895-2938] |
| Buffered range | `player_get_streaming_download_progress(start %, end %)` | D [PH:1976] |
| Variants | `player_foreach_adaptive_variant` (bandwidth, width, height), `player_set_max_adaptive_variant_limit` for HLS and DASH; bandwidth applies only when variants lack a resolution; nothing affordable picks the lowest | D [PH:2394-2433] |
| Rendition change | `player_video_stream_changed_cb(w, h, fps, bitrate)`, set in IDLE, fires just before rendering | D [PH:2245-2261] |
| Rendition change, seen | Fired once for Apple's fMP4 HLS, with 480x270 and fps and bitrate both 0; not within four seconds of Mux's MPEG-TS HLS, which stayed on its first 320x184 variant | H-1 |
| Stream info, seen | `player_get_codec_info` names the video codec `h264` or `hevc`; `player_get_video_stream_info` gives the frame rate (60, 30, 29, 24) and a bitrate for HLS only (6221600, 8001098), 0 for files; `player_get_streaming_download_progress` answers for HTTP files and HLS alike | H-1 |
| Tracks | `player_get_track_count`, `_get_current_track`, `_get_track_language_code` (ISO 639-1, `und`), `_select_track`, for AUDIO and TEXT only; no label or codec | D [PH:2290-2367] |
| Tracks, seen | Matroska test5 lists two audio tracks (`und`, `eng`) and eight text tracks (`en hu de fr es it jp un`); Apple's fMP4 HLS one audio (`eng`) and one text (`en`); files without audio list none. The HLS text track sends `player_subtitle_updated_cb` before any track is selected | H-1 |
| Rate | `player_set_playback_rate(-5..5)` returns `INVALID_OPERATION` on streaming, mutes outside 0..2; Samsung's TV player mutes at any rate but 1.0 | D [PH:2133-2150, SN] |
| HTTP headers | Cookie (`player_set_streaming_cookie`) and User-Agent (`player_set_streaming_user_agent`) only | D [PH:1940-1960]; flutter-tizen documents the same limit [FA] |
| Live | No public API; flutter-tizen reads `player_get_adaptive_streaming_info(IS_LIVE = 19)` and `LIVE_DURATION = 3` through `dlsym` | D [FV: inc/ftpw_video_player.h] |
| Live HLS master | An `EVENT` playlist with no `EXT-X-ENDLIST` opens at its live edge | H (KROMA, AVPlay) [K-landing]; matches `EXT-X-PLAYLIST-TYPE` being unsupported [SG] |

### DRM

| Fact | Known |
|------|-------|
| The public `player.h` has DRM error codes and no DRM call | D [PH:87-90] |
| flutter-tizen plays Widevine and PlayReady through `player_set_drm_handle` and a DRM manager session reached by `dlsym` | D [FH-old: media_player_proxy.cc] |
| `drmplay` needs a partner-level certificate | D by flutter-tizen [FA]; Samsung's namespace page lists `DRMType` as "non-privilege" [STN], so the two disagree |
| PlayReady is deprecated on Tizen TV; Widevine Modular L1 versions per year in section 2 | D [SG] |
| DRM does not play on the TV emulator | D [SN] |

## 5. Gotchas

| Gotcha | Fact | Known |
|--------|------|-------|
| One video player | "Only one video player can be played at one time" | D [TFX-X: Player] |
| More handles | Creating several is allowed but "cannot guarantee proper operation" | D [PH:496] |
| More handles, seen | A second handle preparing while the first is paused interrupts the first (`PLAYER_INTERRUPTED_BY_RESOURCE_CONFLICT`, code 4), which stays PAUSED; `player_start` on the first then plays it and interrupts the second the same way. Both prepare and both report READY; only the decoder moves | H-1 |
| Releasing | AVPlay must stop then close or the next open fails | H (KROMA, AVPlay) [K-calls] |
| Releasing, capi | flutter-tizen unsets every callback, then calls `unprepare`, `stop`, `destroy` | D [FV:209-219]; I that a skipped release blocks the next player, as with AVPlay |
| Cost of each call, seen | `player_create` 17 to 120 ms (the first of a launch is the slowest), `player_set_display` 2 to 129 ms (the first of a launch), `player_prepare_async` 10 to 313 ms; on release `player_stop` 1 to 255 ms, `player_unprepare` 111 to 697 ms, `player_destroy` 4 to 25 ms. All but `player_set_display` now run on the player thread (section 7): before, a video view that went away with its player held the main loop about 180 ms and KROMA's intro view took 151 to 190 ms to unmount; now 1.4 to 4.3 ms | H-1, timers around each call, `rn-tizen perf`, 2026-11-03 |
| Releasing, seen | Unset callbacks, `player_stop` from PLAYING or PAUSED, `player_unprepare`, `player_destroy` returns `PLAYER_ERROR_NONE`, and the next handle prepares; quitting the app without releasing did not stop the next launch from playing | H-1 |
| App to background | No suspend in capi. flutter-tizen's policy: pause if PLAYING; destroy if IDLE, READY, live or the TV is in standby; on resume recreate and seek | D [FH-old: media_player.cc:761-830] |
| Shell lifecycle | The .NET shell forwards `OnPause`/`OnResume` to `rn_tizen_host_pause`/`_resume` | D [RNT: shell/ReactNativeApplication.cs] |
| Screensaver | flutter-tizen on TV calls `screensaver_override_reset(false)` at create and `screensaver_reset_timeout` on a timer, from `libcapi-screensaver.so` via `dlsym` | D [FVO:88-109] |
| Emulator | flutter-tizen's TV players do not run on the TV emulator | D [FV, FH, FA READMEs] |
| Subtitle files | Local paths only: download a remote file first | D [PH:2164, SS] |
| Native subtitle callback | AVPlay ignores silencing subtitles in IDLE; it takes at READY or PLAYING | H (KROMA, AVPlay) [K-engine] |
| Memory | Samsung's memory guide page is gone (404). flutter-tizen's frame path keeps one frame and drops the rest | U for budgets; D for flutter [FV:747-760] |
| Private symbols | flutter-tizen ships one plusplayer build per Tizen range (6.0, 6.5 to 9.0, 10.0) | D [FA] |

## 6. Implications for react-native-tizen

### The backend

Build on capi-media-player with `PLAYER_DISPLAY_TYPE_OVERLAY` on the
Elementary window. It is the only path that is public, keeps the hardware
plane (4K, HDR10, HDR10+), and matches what TizenFX does with an ElmSharp
window. Treat every TV-only export as optional, resolved with `dlsym` and
probed at startup. Leave DRM out of the first version; KROMA does not need it.

| Piece | Where | Does |
|-------|-------|------|
| `TizenPlayer` | `runtime/host/expo/modules/video/` | Owns one `player_h`. `player_set_display` and the short calls on the main loop; `player_create`, `player_prepare_async`, `player_unprepare` and the release on one player thread, which holds the handle meanwhile while the main loop queues the commands that need it (section 4). Callbacks copy their arguments and post through `MainThread` (`ecore_main_loop_thread_safe_call_async`) [RNT: MainThread.cpp:19]. A seek queue keeps one seek in flight and coalesces to the latest target. |
| `ExpoVideo` module | same | `requireNativeModule('ExpoVideo')`: a `VideoPlayer` shared-object class, `isPictureInPictureSupported()` returning false, the cache functions as no-ops [EX: VideoModule.ts, VideoPlayer.tsx:34-61] |
| `VideoView` component | same | Fabric component `ViewManagerAdapter_ExpoVideo_VideoView`, config from `globalThis.expo.getViewConfig('ExpoVideo', 'VideoView')`, methods in `ViewPrototypes['ExpoVideo_VideoView']` [EMC:47-75, 240-255]. `Platform.OS` `tizen` picks `VideoView`, not Android's `SurfaceVideoView` [EX: NativeVideoView.ts] |
| Host prerequisites | `runtime/host/expo/` | Classes with accessors, shared objects with `__expo_shared_object_id__`, events, native views and their methods: done, see `expo-modules-contract.md` |
| Window | `ReactHost::createWindow` | `elm_win_alpha_set(win, EINA_TRUE)` and a transparent root while a video is attached; or keep the window opaque and rely on the hole alone, as flutter-tizen does. Test both on a set and record which works |
| Hole | `VideoView`'s Evas object | An Evas rectangle `(0, 0, 0, 0)` with `EVAS_RENDER_COPY` over the view's box. Evas does not promise this [EV:2214]; it is what flutter-tizen does with GL. Verify first |

Display binding happens once, in IDLE, on the main thread, against the window.
The view only supplies geometry: on each `updateLayout`, `canvasOrigin()` and
the layout size become the ROI. A player created before any `VideoView` mounts
binds to the window anyway and stays invisible (`player_set_display_visible`
false) until a view claims it. Two views showing one player is unsupported;
the last view to mount owns the ROI.

`contentFit` becomes geometry, because DST_ROI fills its box:

| `contentFit` | ROI | Source crop |
|--------------|-----|-------------|
| `contain` (default) | the letterbox rect inside the view, from `player_get_video_size` | none |
| `fill` | the view rect | none |
| `cover` | the view rect | `player_set_video_roi_area` to the view's aspect [PH:2567] |

Recompute on `player_video_stream_changed_cb`, since an adaptive stream can
change size. Skip calls whose rect did not change [K-plane].

### `VideoPlayer` mapping (expo-video 58.0.12 [EX: VideoPlayer.types.ts])

| expo-video | Tizen | Status |
|------------|-------|--------|
| `new VideoPlayer(source)`, `release()` | `player_create`, then IDLE setup; `release` runs `stop`, `unprepare`, `destroy` | direct |
| `source.uri`, `assetId` | `player_set_uri` (http, https, file); a bundled asset resolves to its `file://` path | direct |
| `source.headers` | Cookie and User-Agent only; other headers are dropped with a warning | partial |
| `source.drm`, `useCaching`, `metadata` | none in phase one | unsupported |
| `source.contentType` | ignored; whether capi needs a DASH hint is U | U |
| `play()`, `pause()` | `player_start`, `player_pause` | direct |
| `replace(source)`, `replaceAsync` | `unprepare`, `set_uri`, `prepare_async` on the same handle; the display stays bound | derived |
| `currentTime` get / set, `seekBy`, `replay()` | `get_play_position`; `set_play_position` through the seek queue | derived |
| `seekTolerance` | `accurate = true` only when both tolerances are 0 | derived |
| `duration`, `isLive` | `player_get_duration`; `isLive` from the private live query, else false | derived |
| `loop`, `muted`, `volume` | `player_set_looping`, `_set_mute`, `_set_volume(v, v)` | direct |
| `playbackRate` | `player_set_playback_rate`; throws on streams [PH:2133] | partial |
| `status` | `idle` before prepare, `loading` during prepare or a buffering dip, `readyToPlay` at prepared or 100 %, `error` from `player_error_cb` or a failed prepare | derived |
| `playing` | state is PLAYING | direct |
| `timeUpdateEventInterval` | an `ecore_timer` polling the position | derived |
| `bufferedPosition` | download progress end % times duration | derived, I |
| `bufferOptions` | `minBufferForPlayback` to `prebuffer_ms`, applied in IDLE; the rest has no equivalent | partial |
| `audioTrack`, `availableAudioTracks` | track count, language, select; label is the language until the private v2 track info adds codec and channels | derived |
| `subtitleTrack`, `availableSubtitleTracks` | the same for TEXT; text arrives on `player_subtitle_updated_cb` and the view draws it | derived |
| `videoTrack`, `availableVideoTracks` | adaptive variants plus the stream-changed callback; `videoRange` is U | partial |
| `maxResolution` | `player_set_max_adaptive_variant_limit(-1, w, h)` | direct for HLS and DASH |
| `keepScreenOnWhilePlaying` | the private screensaver calls while PLAYING | derived |
| `generateThumbnailsAsync` | `player_capture_video` only grabs the current frame [PH:1911] | unsupported |
| `preservesPitch`, `scrubbingModeOptions`, `audioMixingMode`, `staysActiveInBackground`, `showNowPlayingNotification`, live offsets | no equivalent | unsupported |

| Event | Source |
|-------|--------|
| `statusChange` | state and buffering transitions above, with `error` on failure |
| `playingChange` | entering or leaving PLAYING |
| `timeUpdate` | the position timer, with `bufferedPosition` |
| `playToEnd` | `player_completed_cb` |
| `sourceLoad`, `available*TracksChange` | `player_prepared_cb`, after reading duration and tracks |
| `sourceChange` | `replace` |
| `audioTrackChange`, `subtitleTrackChange` | after a select returns success |
| `videoTrackChange` | `player_video_stream_changed_cb` |
| `playbackRateChange`, `volumeChange`, `mutedChange` | the setters, after the call succeeds |

### `VideoView` mapping [EX: VideoView.types.ts]

| Prop or method | Tizen | Status |
|----------------|-------|--------|
| `player` | shared-object id; the view takes the ROI for that player | direct |
| `contentFit` | table above | derived |
| `nativeControls` | no native controls exist; ignored | unsupported |
| `onFirstFrameRender` | no first-frame callback; fire on the first PLAYING after prepare | derived, I |
| `enterFullscreen`, `exitFullscreen` | ROI to the whole window and back | derived |
| PiP props and methods | none | unsupported |
| `surfaceType`, `useExoShutter`, Android and iOS-only props | ignored | unsupported |
| style: `borderRadius`, `transform`, opacity | reach the hole only, never the plane | unsupported |

### What KROMA's direct play needs from it

| Need | Consequence | Known |
|------|-------------|-------|
| KROMA plays HEVC in MKV, MP4 and TS directly | allowed on every 2018+ set [SV] | D |
| KROMA's native caps claim Dolby Vision and deny AV1 [K-caps] | on Tizen Dolby Vision is false and AV1 true from 2020 (UHD) or 2021 (all) | D [TR, SV-2020, SV-2021] |
| DTS, TrueHD and FLAC tracks | send them to the server's transcode | D for DTS [SV]; H for the rest [K-caps] |
| Its HLS master is an `EVENT` playlist | expect the live-edge open seen in AVPlay; seek to the anchor after PLAYING | H [K-landing], U for capi |
| It docks and moves the video (`use-docked-player`) | each move is an ROI update, not a transform; `OVERLAY_SYNC_UI` (6.5+) is the path if moves must track UI frames | I [PH:113-122] |
| Its stream URLs | auth must ride in the URL; only Cookie and User-Agent headers pass | D [PH:1940-1960] |

### Verify on a TV before building on it

| # | Check | Result on H-1 |
|---|-------|---------------|
| 1 | `player_set_display(OVERLAY, elm_win)` from `libreact-native-tizen.so` shows video under the window | Yes: every display call returns `PLAYER_ERROR_NONE` and the clip plays inside the view's box, and fills the screen in fullscreen (checked on screen) |
| 2 | Which of window alpha, the `EVAS_RENDER_COPY` hole, or both, makes it visible | Both, once a page background sits behind the video (section 3). The host always uses both |
| 3 | HEVC Main10 HDR10 in MKV switches the panel to HDR on the overlay | HEVC Main10 in MKV and HEVC HDR10 in MP4 prepare and play (no public HDR10 MKV was found). The panel switches to HDR for the HDR10 MP4 (checked on screen) |
| 4 | Seek and audio-track selection at READY on capi, against the AVPlay refusals | Both accepted at READY (section 4) |
| 5 | Which callbacks arrive off the main loop | All seven seen, off the main loop (section 4) |

Record each result here with the set's model, year and Tizen version.

## 7. What react-native-tizen implements

`runtime/host/expo/modules/video/`, on the design of section 6, run on H-1 with
every source in the sources table.

| Piece | Files | Does |
|-------|-------|------|
| `TizenPlayer` | `TizenPlayer.cpp`, `TizenPlayerEvents.cpp`, `TizenPlayerDisplay.cpp`, `TizenPlayerLifecycle.cpp`, `TizenPlayerSettings.cpp`, `PlayerThread.cpp` | One handle. Created on the player thread, then bound to the window on the main loop and given its URI and its User-Agent and Cookie headers in IDLE, then prepared on the player thread. While that thread holds the handle (create, prepare, the unprepare of a `replace`, the release), commands and callback effects wait in order and run when it hands the handle back; one thread for every player keeps a release ahead of the next create. One seek in flight, the next coalesced to the latest target; play and pause wait for it. A seek or track refused at READY is retried once PLAYING (capi did not refuse either, section 4). Progress polls on an `ecore_timer` at `timeUpdateEventInterval`, or every 0.5 s for the getters. Release hides the plane on the main loop, then unsets callbacks, stops, unprepares and destroys on the player thread |
| `VideoPlayer` | `VideoPlayer.cpp`, `VideoPlayerEvents.cpp`, `VideoTracks.cpp` | The SharedObject. Getters read a snapshot the main loop keeps; `currentTime` extrapolates from the last report at the playback rate. Setters update the snapshot and queue the call. Prepare waits one JavaScript turn, so settings made right after `new` (KROMA's `bufferOptions`) reach the player in IDLE. Events go out on the JavaScript thread once the object is paired |
| `ExpoVideo` | `VideoModule.cpp` | The module and the `VideoPlayer` class. `isPictureInPictureSupported` is false, the cache functions do nothing |
| `VideoView` | `VideoView.cpp`, `VideoFit.cpp`, `SubtitleOverlay.cpp` | The view's box in window pixels (`canvasRect`, at the surface's point scale) is the ROI, recomputed on every layout and on a rendition change, fitted by `contentFit`. A hole over the box, a fullscreen hole on layer 2000, `onFirstFrameRender` at the first PLAYING it shows, the selected text track drawn over the bottom of the box |
| Window | `VideoWindow.cpp`, `runtime/host/src/HostWindow.cpp` | Window alpha and each view's hole, the only composition that works over a page background (section 3). Window alpha is on only while a view shows a player that holds an overlay |
| Simulator | `simulator/shims/src/player.c` | `player_create` fails, so a player's status is `error` with "This device has no media player: player_create returned PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE" and nothing is drawn or cleared |

| expo-video surface | On Tizen |
|--------------------|----------|
| `play`, `pause`, `replace`, `replaceAsync`, `release`, `seekBy`, `replay` | done |
| `status`, `playing`, `currentTime` (read and write), `duration`, `bufferedPosition` (-1 until the player tells) | done |
| `loop`, `muted`, `volume`, `timeUpdateEventInterval` | done |
| `playbackRate` | passed to `player_set_playback_rate`, which capi refuses on streams |
| `bufferOptions` | `minBufferForPlayback` only, as the prebuffer of the next prepare |
| `seekTolerance` | accurate seeks only when both tolerances are 0 |
| `maxResolution` | `player_set_max_adaptive_variant_limit` |
| `audioTrack`, `availableAudioTracks`, `subtitleTrack`, `availableSubtitleTracks` | done; ids `audio-<index>` and `text-<index>`, labels the language code or `Track <n>`. No subtitle shows until one is selected; `null` stops the drawing, since capi cannot deselect |
| `videoTrack`, `availableVideoTracks` | the stream playing: size, codec name as `mimeType`, frame rate, HLS bitrate. `videoRange` left out: U |
| `source.headers` | User-Agent and Cookie; any other header is logged and dropped |
| `isLive`, `currentLiveTimestamp`, `currentOffsetFromLive`, `isExternalPlaybackActive` | false, null, null, false |
| `preservesPitch`, `keepScreenOnWhilePlaying`, `showNowPlayingNotification`, `staysActiveInBackground`, `allowsExternalPlayback`, `audioMixingMode`, `scrubbingModeOptions`, `targetOffsetFromLive` | kept and read back, no effect |
| `generateThumbnailsAsync`, `startPictureInPicture` | reject: unsupported |
| Events | every event in section 6 but `isExternalPlaybackActiveChange` |
| `VideoView` | `player`, `contentFit`, `enterFullscreen`, `exitFullscreen` (and their events), `onFirstFrameRender`; other props are not declared, so React Native drops them |

Not handled: two views showing one player (the last to lay out owns the ROI);
the app's Back during fullscreen (the app calls `exitFullscreen`); a player the
app never releases outlives its JavaScript runtime until the process exits.

## Sources

| Tag | Source |
|-----|--------|
| PH | `~/tizen-studio/platforms/tizen-9.0/tizen/rootstraps/tizen-9.0-device.core/usr/include/media/player.h` |
| EV | same rootstrap, `evas-1/Evas_Legacy.h` |
| EW | same rootstrap, `elementary-1/elm_win_legacy.h` |
| NG | Tizen native guide, [Media Playback](https://docs.tizen.org/application/native/guides/multimedia/media-playback/) ([source](https://github.com/Samsung/tizen-docs/blob/master/docs/application/native/guides/multimedia/media-playback.md)) |
| TFX-X | `Tizen.NET.API9` 9.0.0.16760 XML docs: `Tizen.Multimedia.xml`, `Tizen.Multimedia.MediaPlayer.xml` (NuGet cache) |
| TFX-D9 | TizenFX `API9`, [`src/Tizen.Multimedia/Common/Display.cs`](https://github.com/Samsung/TizenFX/blob/API9/src/Tizen.Multimedia/Common/Display.cs) |
| TFX-P | TizenFX `API9`, [`src/Tizen.Multimedia.MediaPlayer/Player/Player.cs`](https://github.com/Samsung/TizenFX/blob/API9/src/Tizen.Multimedia.MediaPlayer/Player/Player.cs) (`Player()`, `PrepareAsync()`, `Unprepare()`) and [`Interop/Interop.Player.cs`](https://github.com/Samsung/TizenFX/blob/API9/src/Tizen.Multimedia.MediaPlayer/Interop/Interop.Player.cs) (`PlayerHandle.ReleaseHandle`) |
| TFX-I | TizenFX `main`, [`Interop.Display.cs`](https://github.com/Samsung/TizenFX/blob/main/src/Tizen.Multimedia.MediaPlayer/Interop/Interop.Display.cs) |
| SG | Samsung, [General Specifications](https://developer.samsung.com/smarttv/develop/specifications/general-specifications.html) |
| SV-year | Samsung, `https://developer.samsung.com/smarttv/develop/specifications/media-specifications/<year>-tv-video-specifications.html`, e.g. [2024](https://developer.samsung.com/smarttv/develop/specifications/media-specifications/2024-tv-video-specifications.html) |
| SA | Samsung, [Adaptive Streaming](https://developer.samsung.com/smarttv/develop/guides/multimedia/adaptive-streaming.html) |
| SS | Samsung, [Subtitles](https://developer.samsung.com/smarttv/develop/guides/multimedia/subtitles.html) |
| SP | Samsung, [Playback Using AVPlay](https://developer.samsung.com/smarttv/develop/guides/multimedia/media-playback/using-avplay.html) |
| SN | Samsung, [.NET Multimedia guide](https://developer.samsung.com/smarttv/develop/tizen-net-tv/guides/multimedia.html) |
| STN | Samsung, [`Tizen.TV.Multimedia` namespace](https://developer.samsung.com/smarttv/develop/api-references/tizenfx-tv-api-references/tizen.tv.multimedia.html) |
| STP | Samsung, [`Tizen.TV.Multimedia.Player`](https://developer.samsung.com/smarttv/develop/api-references/tizenfx-tv-api-references/tizen.tv.multimedia/player-class.html) |
| FV | flutter-tizen [`packages/video_player`](https://github.com/flutter-tizen/plugins/tree/3a8777d0f1b46eb2ba69daf335e26da2dd67f06f/packages/video_player) at `3a8777d`, `tizen/src/video_player.cc` unless named |
| FVO | the same plugin at [`2574d68`](https://github.com/flutter-tizen/plugins/blob/2574d686bbb8a8deb76af6291e32b911dcc03a90/packages/video_player/tizen/src/video_player.cc), before its wrapper library |
| FH | flutter-tizen [`packages/video_player_videohole`](https://github.com/flutter-tizen/plugins/tree/3a8777d0f1b46eb2ba69daf335e26da2dd67f06f/packages/video_player_videohole) at `3a8777d` |
| FH-old | the same plugin at [`05634a1`](https://github.com/flutter-tizen/plugins/tree/05634a18b4680638a4605219e8b4130b37475ef1/packages/video_player_videohole/tizen/src), with its `dlsym` proxy |
| FA | flutter-tizen [`packages/video_player_avplay`](https://github.com/flutter-tizen/plugins/tree/3a8777d0f1b46eb2ba69daf335e26da2dd67f06f/packages/video_player_avplay) README and `project_def.prop` |
| FE | flutter-tizen [`FlutterApplication.cs`](https://github.com/flutter-tizen/flutter-tizen/blob/master/embedding/csharp/Tizen.Flutter.Embedding/FlutterApplication.cs) |
| EX | `expo-video` 58.0.12, `src/` |
| EMC | `expo-modules-core` 58.0.23, `src/NativeViewManagerAdapter.native.tsx` |
| TR | TechRadar, [Dolby Vision on Samsung TVs: why isn't it supported?](https://www.techradar.com/news/dolby-vision-on-samsung-tvs-why-isnt-it-supported) |
| SAT | Samsung support, [How to use Dolby Atmos with your Samsung Soundbar](https://www.samsung.com/latin_en/support/tv-audio-video/how-to-use-dolby-atmos-with-your-samsung-soundbar/) |
| RNT | this package: `runtime/host/src/ReactHost.cpp`, `runtime/host/src/HostWindow.cpp`, `runtime/host/src/threading/MainThread.cpp`, `runtime/host/expo/ExpoModuleBuilder.h`, `shell/ReactNativeApplication.cs` |
| H-1 | Samsung GQ75LS03DAUXZG (The Frame, 2024 model year, upgraded to Tizen 9.0, armv7, `wayland_egl`), 2026-11-03, through `example/src/screens/video/` (`main.ts`, `probe.ts`) and its logs. Sources: Jellyfin's `Test Jellyfin 1080p AVC 3M.mp4` and `Test Jellyfin 1080p HEVC HDR10 10M.mp4` (repo.jellyfin.org/test-videos), Mux `x36xhzz.m3u8`, Apple `img_bipbop_adv_example_fmp4/master.m3u8`, jellyfish `3-mbps-hd-hevc.mkv` and `10-mbps-hd-hevc-10bit.mkv`, Matroska `test5.mkv` (ietf-wg-cellar/matroska-test-files) |
| K-caps | KROMA [`packages/core/src/hevc/runtime-caps.ts:64-84`](https://github.com/kromatv/kroma/blob/1d270635adcc9f3a81283e04f06336845d8c5985/packages/core/src/hevc/runtime-caps.ts#L64-L84) |
| K-landing | KROMA [`packages/tv/src/features/playback/player/avplayLanding.ts`](https://github.com/kromatv/kroma/blob/1d270635adcc9f3a81283e04f06336845d8c5985/packages/tv/src/features/playback/player/avplayLanding.ts) |
| K-direct | KROMA [`packages/tv/src/features/playback/player/avplayDirect.ts`](https://github.com/kromatv/kroma/blob/1d270635adcc9f3a81283e04f06336845d8c5985/packages/tv/src/features/playback/player/avplayDirect.ts) |
| K-calls, K-plane, K-engine | KROMA [`packages/tv/src/features/playback/player/avplayCalls.ts`](https://github.com/kromatv/kroma/blob/1d270635adcc9f3a81283e04f06336845d8c5985/packages/tv/src/features/playback/player/avplayCalls.ts), [`avplayPlane.ts`](https://github.com/kromatv/kroma/blob/1d270635adcc9f3a81283e04f06336845d8c5985/packages/tv/src/features/playback/player/avplayPlane.ts), [`avplayEngine.ts`](https://github.com/kromatv/kroma/blob/1d270635adcc9f3a81283e04f06336845d8c5985/packages/tv/src/features/playback/player/avplayEngine.ts) |
