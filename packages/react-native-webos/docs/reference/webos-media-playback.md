# webOS media playback

What an LG webOS TV gives a native app for video, and what `expo-video` can be
built on. Read before touching anything that plays media.

LG documents formats and the web `<video>` options, and nothing about native
playback. Almost everything below was read from source: webOS OSE (LG's open
edition, which is not what TVs ship), LG's Flutter plugins (webOS 26 only), a
symbol index of real firmware, and native apps that ship on TVs.

How a fact is known: **D** documented on LG's developer site. **S** read from
source, the file named; OSE code shows the design, not always what a TV runs.
**S-report** read from a project's source or docs where its author records
what their own TV did, not checked by this project. **I** inferred, the reason
given. **U** unknown.

**Nothing in this file has been verified on a TV by this project.** Version
numbers are platform releases (4.10.0, 11.2.0). LG's marketing names map as
5.x = 2020, 6.x = 2021, 7.x = webOS TV 22, 8.x = 23, 9.x = 24, 10.x = 25,
11.x = 26 (I: from [LG-SP]'s year table, [FW]'s release list and [PLX-C]).

## 1. The media stacks

| Stack | What it is | Input | A jailed third-party app |
|-------|-----------|-------|--------------------------|
| `com.webos.media` (uMediaServer) called over luna-service2 | A service that starts one pipeline process per `load`. On TVs the pipeline is `starfish-media-pipeline` (S-report [PLX-U:10]); on OSE it is `g-media-pipeline`, a GStreamer `playbin` (S [GMP: src/player/UriPlayer.cpp:552]) | A URI: file, http(s), HLS, DASH. The pipeline fetches, demuxes, decodes, renders subtitles | Allowed by LG's role template (section 1.2). nuvio ships on it: its author's 4.10 set, and user reports from 3.4.3 to 2024 models (S-report [NUV-R:157-161]). plx-native probed it on 5.4.2 and 11.1.0 (S-report [PLX-U:26-28]) |
| `libNDL_media.so.1` (`NDL_Media*`) | A C API over `uMediaClient`, in a stub that `dlopen`s its `_impl` library (S [FW], S-report [JWN-N]) | Same URI path | What LG's own Flutter plugins call, "webOS 26 media or above" (S [FL: README.md]) |
| `libumedia_api.so.1` (`uMediaServer::uMediaClient`, C++) | The client class the web engine uses for `<video>` on OSE (S [CHR:1189]) | Same URI path | No native app found using it. Its C++ ABI changed (section 1.1) |
| `libumedia_api_c.so.1` | A few C calls: create, load, play, pause, seek, unload, destroy (S [FW]) | Same URI path | No events and no window: not enough for a player |
| `libplayerAPIs.so.1` (`StarfishMediaAPIs`, "SMP") | The same pipeline code loaded into the app's process (S [UL: include/starfish-media-pipeline/StarfishMediaAPIs.h:37-86]) | Elementary streams through `Feed` (`mediaTransportType` `BUFFERSTREAM`). A URI path exists in the library (`SetUri`, `SelectTrack`, `setSubtitleSource` exported, S [FW]) | Kodi, Moonlight (ss4s) and plx-native feed it elementary streams (S [KODI-M:699], [SS4S-P:384]). No working in-process URI load was found |
| `libNDL_directmedia.so.1` | Elementary-stream player | H.264, H.265, VP9, AV1 access units; PCM or Opus audio (S [UL: include/libndl-media/NDL_directmedia_types.h:118-133]) | ss4s, jellyfin-webos-native. The app demuxes and keeps the clock (S-report [JWN-N]) |
| GStreamer 1.x | `libgstreamer-1.0.so.0` on every firmware (S [FW]); LG's decoder and sink elements (`decproxy`, `lxvideosink`, `lgaudiosink`) on one set (S-report [JWN-M]) | Anything `playbin` takes | U: no native app found that builds its own pipeline. SMP builds one inside the app's process, so the libraries load in the jail (I from [UL: include/player-factory/custompipeline.hpp:13-33]) |

### 1.1 What each firmware has

From webosbrew's index of the libraries and exported symbols of 14 firmware
images, releases 1.2.0, 1.4.0, 2.2.3, 3.4.0, 3.9.2, 4.4.2, 4.10.0, 5.3.1,
6.4.0, 7.4.0, 8.3.0, 9.2.0, 10.2.0 and 11.2.0. All S [FW]. A symbol's presence
says the binary loads, not that the call works.

| Library or symbol | Present in |
|-------------------|-----------|
| `libumedia_api.so.1`, `libumedia_api_c.so.1`, `libplayerAPIs.so.1`, `libNDL_media.so.1`, `libgstreamer-1.0.so.0`, `libluna-service2.so.3` | all 14 |
| `NDL_MediaInit`, `Load`, `Play`, `Pause`, `SeekTo`, `SetPlaybackRate`, `SetDisplayWindow`, `SubscribeEvents`, `GetError`, `Quit` | all 14 |
| `NDL_MediaUnload`, `NDL_MediaGetCurrentPosition`, `NDL_Media_DL_Initialize` | 2.2.3 and later |
| `NDL_MediaLoadWithOption`, `NDL_MediaSelectTrack`, `NDL_MediaSetSubtitle{Enable,Source,Sync,Color,Encoding,FontSize,Position}`, `NDL_MediaDRM{Init,Load,SendMessage,SendMessageAsync,GetError,GetRightsError,IsLoaded,Unload,Quit}` | 3.4.0 and later |
| `NDL_MediaMuteDisplay` | 5.3.1 and later |
| `NDL_MediaSetPlayer`, `NDL_MediaPreload` | 6.4.0 and later |
| `NDL_MediaInitWithWindowId`, `NDL_MediaSetDrmOperation`, `NDL_MediaUnsubscribeEvents` | 11.2.0 only |
| `uMediaClient` methods taking `std::string` | old ABI up to 3.9.2, `std::__cxx11` from 4.4.2: one binary cannot call both |
| `uMediaClient::setWindowId`, `setAudioMute`, `setSink`, `setDrmOperation` | 5.3.1 and later |
| `uMediaClient::setDisplayWindow`, `switchToFullscreen` (OSE's, [UMS: include/public/uMediaClient.h:397-399]) | none of the 14 |
| `StarfishMediaAPIs::Load(const char*, void(*)(int, long long, const char*))`, `Play`, `Pause`, `Seek`, `Unload`, `SetUri`, `SelectTrack`, `SetPlayRate`, `setSubtitleSource`, `setSubtitleEnable` | all 14 |
| `StarfishMediaAPIs::Load` with a `void*` context, `Preload`, `setVolume` | 3.4.0 and later |
| `StarfishMediaAPIs::setWindowId` | 5.3.1 and later |
| `libAcbAPI.so.1` (`AcbAPI_*`) | 2.2.3 to 4.10.0, gone from 5.3.1 |
| `wl_webos_foreign_interface` in `libwayland-webos-client.so.1`; `SDL_webOSCreateExportedWindow`, `SetExportedWindow`, `ExportedSetCropRegion`, `ExportedSetProperty`, `DestroyExportedWindow` | 5.3.1 and later |
| `libNDL_directmedia.so.1` | 3.9.2 and later. `NDL_DirectVideoOpen` and `NDL_DirectAudioOpen` (API 1) on 3.9.2 to 4.10.0; `NDL_DirectMediaLoad` (API 2) from 5.3.1; `NDL_DirectMediaSetWindowId` from 6.4.0 |
| `libndl-directmedia2.so.1` (`NDL_Esplayer*`) | 2.2.3 and 3.4.0 only |

`StarfishMediaAPIs` is a C++ class whose size differs per firmware (the stub
header pads it with 4 KB), and `Feed` returns a `std::string`, so it follows
the string ABI change too: ss4s builds it three times, for below 4, 4, and 5
and later. S [UL: StarfishMediaAPIs.h:79-86], [SS4S: modules/webos/smp/CMakeLists.txt].

### 1.2 Permission: the luna role

| Fact | Known |
|------|-------|
| LG's role template for a native app allows the bus names `com.webos.media.client.*`, `com.webos.rm.client.*`, `com.webos.pipeline.*` and the app id, with outbound calls from the first two to `com.webos.media` | S [ROLE: templates/NativeApp.json:4-22] (OSE's role generator; `NDK.pub` and `NDK.prv` say the same in the older format) |
| On OSE every `com.webos.media/*` method is in the ACG group `media`, and a service named `com.webos.media.client.*` is granted that group | S [UMS: files/sysbus/com.webos.media.api.json:2-35, com.webos.media.client.perm.json:2-4] |
| On a TV the Developer Mode installer writes `/var/palm/ls2-dev/roles/pub/<appId>.json` with those names, keyed on the binary's exact path. No `requiredPermissions` entry is needed for playback | S-report [PLX-D:446-455] (webOS 4.5), [NUV:1867] (C9, 4.10), [JWN-N] |
| So the client registers with `LSRegister("com.webos.media.client.<suffix>", ...)`. Another name is refused: `-1027 Invalid permissions`, or `Denied method call` on the first `load` | S-report [NUV:1862-1880], [PLX: rust-modules/play-uri/src/register_ladder.rs:5-11] |
| `uMediaClient` registers as `com.webos.media.client.<uid>` by itself | S [UMS: src/media_client/uMediaClient.cpp:36,72-79] |
| LG's `libhelpers.so.2` cannot register that name: `HLunaServiceCall(uri, payload, HContext *)` is its one way to call, and `HContext` holds a callback, user data, a `multiple` and a `pub` flag and a token, no handle and no name | S [UL: include/webos-helpers/libhelpers.h:26-65] |
| So a named client calls luna-service2 itself. A process that has `libhelpers.so.2` has it loaded: `libluna-service2.so.3` and `libglib-2.0.so.0` are in the `needed` list of `libhelpers.so.2.0.0` on each of the 14 firmware images | S [FW: `<firmware>/libhelpers.so.2.0.0.json`] |
| The calls a named client needs: `LSErrorInit`, `LSErrorFree`, `LSRegister(name, &handle, &error)`, `LSGmainContextAttach(handle, context, &error)`, `LSCall` and `LSCallOneReply(handle, uri, payload, callback, context, &token, &error)`, `LSCallCancel(handle, token, &error)`, `LSMessageGetPayload(message)`, `LSUnregister(handle, &error)`. `LSError` is `{int error_code; char *message; const char *file; int line; const char *func; void *padding; unsigned long magic;}`; the callback is `bool (*)(LSHandle *, LSMessage *, void *)`; a token is an `unsigned long` | S [LS2H:159-169, 312-313, 357, 383, 422, 450, 509-554] |
| GLib's side of it: `g_main_context_new`, `g_main_loop_new(context, is_running)`, `g_main_loop_run`, `g_main_loop_quit`, `g_main_context_invoke(context, function, data)`, whose function returns a `gboolean` | S [GLIBH:202, 490-498, 679-688, 990-992]. I, from GLib's documentation as remembered and not opened here: `g_main_context_invoke` runs the function on the thread that iterates the context, which is why the host makes every luna-service2 call through it |
| plx-native tries the named client first and an anonymous one (`LSRegister(NULL)`) second; its user's 5.4.2 set ended on the named client | S [PLX: rust-modules/play-uri/src/register_ladder.rs:1-17]; S-report [PLX-U:26] |
| nuvio registers `com.webos.media.client.<its app id>`, then the same name with `.<pid>` when the first is refused | S [NUV:1862-1890], [NUV: src/app_id.h:13-15] |
| `com.webos.service.tv.display` (`setDisplayWindow`, `setCustomDisplayWindow`) refuses that identity: "Not permitted to send" | S-report [NUV:524-531] (C9) |
| Whether a store install or a Homebrew Channel install gets the same role file | U. [PLX-D:461-467] names two jail profiles chosen by install prefix; no source records the second one's role file |
| `StarfishMediaAPIs`' constructor calls `exit(0)` when the process does not match the role's `exeName` | S-report [NUV-R:352-354] (C9). Kodi and Moonlight construct it from installed apps without that problem (I from their shipping) |

### 1.3 `com.webos.media` over luna-service2

`LSRegister`, `LSGmainContextAttach` to a GLib context the app pumps, then
`LSCall("luna://com.webos.media/<method>", json, callback, ...)`. A call keeps
its token, so `subscribe`'s callback receives every later event. S [PLX:
rust-modules/platform/src/webos.rs:792-833, webos/ls2/uri.rs:129-149].

| Method | Payload | Known |
|--------|---------|-------|
| `load` | `{"uri":U,"type":"media","payload":{...}}`, answers `{"returnValue":true,"mediaId":M}` | S [UMS: src/server/uMediaserver.cpp:397-498], [PLX-P:88-102], [NUV:2612-2643] |
| `subscribe` | `{"mediaId":M}`; plx-native adds `"subscribe":true`. OSE adds the caller with `LSSubscriptionAdd` whatever the payload says, and answers `{"subscription":true}` | S [UMS: uMediaserver.cpp:810-973, src/ums_connector/UMSConnector_impl.cpp:605-628], [PLX-P:218-220] |
| `play`, `pause`, `unload` | `{"mediaId":M}` | S [UMS: uMediaClient.cpp:869-953] |
| `seek` | `{"mediaId":M,"position":<ms>}` | S [UMS: uMediaClient.cpp:930-936] |
| `setPlayRate` | `{"mediaId":M,"playRate":1.5,"audioOutput":true}` | S [UMS: uMediaClient.cpp:1004-1011], [NUV:3924] |
| `setVolume` | `{"mediaId":M,"volume":0..100,"ease":{"duration":<ms>,"type":"Linear"}}`; the other ease types are `InCubic` and `OutCubic` | S [UMS: uMediaClient.cpp:1189-1200, include/public/uMediaTypes.h:55-62] |
| `notifyForeground`, `notifyBackground` | `{"connectionId":M}` | S [UMS: uMediaClient.cpp:961-977] |
| `selectTrack` | `{"mediaId":M,"type":"audio" or "text","index":n}` | S [PLX-P:227-231], [NUV:3771]. Not in OSE's method list, so TV only |
| `setSubtitleEnable`, `setSubtitleSource` | `{"mediaId":M,"enable":bool}`; `{"mediaId":M,"uri":U,"preferredEncodings":["UTF-8"]}` | S [NUV:3885-3896], [PLX-P:232-234]. TV only |
| `setWindowId` | `{"mediaId":M,"windowId":W}` | S [PLX-P:235-238]. After `loadCompleted` it is too late on 5.4.2 (S-report [PLX-U:26]) |
| `setDisplayWindow`, `switchToFullScreen` | OSE only. A 4.10 set answers `Unknown method "setDisplayWindow"` | S [UMS: com.webos.media.api.json:19,27]; S-report [NUV:1578-1586] |

uMediaServer unloads a pipeline when the client that loaded it leaves the bus,
so the registration must outlive playback and the last `unload`. S [UMS:
uMediaserver.cpp:488], [PLX: webos/ls2/uri.rs:18-22].

The `payload` object, shared by this stack, `NDL_MediaLoadWithOption` and the
web engine:

| Field | Meaning | Known |
|-------|---------|-------|
| `mediaTransportType` | `URI` (default), `HLS`, `MPEG-DASH`, `MSIIS`, `UDP`, `RTP`, `RTSP`; `BUFFERSTREAM` and `DIRECTMEDIA-ES-PLAYER` for fed streams. A sibling of `option` | D [LG-MO] for `URI` and `HLS`; S [CHR:983-1043], [FL-P:274-288] for the rest |
| How it is chosen | LG's plugin and the web engine set `HLS` when the URL contains `m3u8`, and send nothing for a plain file | S [FL-P:274-288, 316-320], [CHR:1018-1043] |
| `option.appId` | The app id. uMediaServer reads it to register the media with the display controller | S [UMS: uMediaserver.cpp:448,482] |
| `option.windowId` | The exported window's id (section 2). webOS 5 and later | S [CHR:1003], [FL-P:327-329], [KODI-M:657] |
| `option.transmission.httpHeader` | `{"referer":..,"userAgent":..,"cookies":..}`: the only three headers | S [CHR:995-998]. S-report [NUV:2569-2594]: five other spellings failed with error 203 on a C9, this nesting played; only `referer` was proven. LG's plugin states `httpHeaders` is not supported [FL: README.md] |
| `option.transmission.playTime.start` | Start position in ms | D [LG-MO]; S [FL-P:333-341] |
| `option.bufferControl` | `{"userBufferCtrl":false}`, or `true` with `preBufferByte`, `bufferMinLevel`, `bufferMaxLevel` | S [CHR:999], [NUV:2616-2626] |
| `option.useSeekableRanges`, `needAudio`, `preload`, `seekMode:"accurate"` | Web engine defaults | S [CHR:1006-1048] |
| `option.mediaFormat.type` | `video` (default), `audio`, `video_only` | D [LG-MO] |
| `option.adaptiveStreaming` | `maxWidth` (default 1920), `maxHeight` (1080), `adaptiveResolution`, `seamlessPlay`, `audioOnly`, `bps.start` | D [LG-MO]. `bps.maximum` and `bps.minimum` are plx-native's, untested (S [PLX-P:40-45]) |
| `option.drm` | Section 3 | D [LG-MO], S [FLD-P:343-354] |

### 1.4 `libNDL_media`

No header is public. These call shapes are read from LG's plugins; return
types are I from their `!= 0` checks.

```c
int NDL_MediaSetPlayer(int slot);          /* 0..4: the player the next call addresses */
int NDL_MediaInit(/* LG passes 0 */);
int NDL_MediaInitWithWindowId(/* LG passes 0 */, const char *windowId);
int NDL_MediaSubscribeEvents(void (*callback)(char *eventJson));
int NDL_MediaUnsubscribeEvents(void);
int NDL_MediaLoadWithOption(const char *uri, const char *payloadJson);
int NDL_MediaPlay(void);   int NDL_MediaPause(void);   int NDL_MediaUnload(void);
int NDL_MediaSeekTo(long long milliseconds);
int NDL_MediaSetPlaybackRate(double rate);
long long NDL_MediaGetCurrentPosition(void);
int NDL_MediaSelectTrack(const char *type, int index);
int NDL_MediaSetSubtitleEnable(bool enable);   int NDL_MediaSetSubtitleSync(int offset);
int NDL_MediaMuteDisplay(bool mute);
const char *NDL_MediaGetError(void);
```

S [FL-P:159-220, 361-366, 416-513], [FLD-P:388-390, 487-520], [FL-A:128, 470].

| Fact | Known |
|------|-------|
| The event callback gets the same JSON as a `subscribe` event (section 4) and has no user-data argument; it runs on uMediaClient's thread | S [FL: webos/webos_player.h:139-142, webos_player.cc:519-610] |
| Five players per app; a sixth evicts the oldest | S [FL: README.md], [FL: webos_player.h:26] |
| Without a window id the library falls back to `SDL_webOSCreateExportedWindow`, which gives a null id in a non-SDL app and crashes `TVDisplay::Initialize`. `NDL_MediaInitWithWindowId` avoids it, and exists on 11.2.0 only | S [FL-P:171-177]; S [FW] |
| LG's plugin never calls `NDL_MediaQuit`: it deletes the `uMediaClient` without joining its callback thread | S [FL-P:225-229] |
| `NDL_MediaSetVolume` does not exist; LG's plugin reports `setVolume` as unsupported | S [FW], [FL-P:485-493] |
| `NDL_Media_DL_Initialize()` must run first or every entry point traps | S-report [JWN-N], on a 2024-kernel set whose release the notes do not give. LG's plugin does not call it (S [FL-P]) |
| `pkg-config ndlmedia` in LG's NDK resolves to this library | I: the plugin's calls are this library's exports [FW]; the NDK itself (a 325 MB installer) was not opened |

### 1.5 `StarfishMediaAPIs` and `NDL_directmedia`, for the record

`StarfishMediaAPIs(const char *uid)`, `Load(payload, callback, context)`,
`Feed(json)`, `Play`, `Pause`, `Seek(const char *millis)`, `SetPlayRate(json)`,
`flush`, `pushEOS`, `Unload`, `notifyForeground`, `setWindowId`, `setHdrInfo`,
`getCurrentPlaytime`. `Load` takes
`{"args":[{"mediaTransportType":"BUFFERSTREAM","option":{"appId":..,"windowId":..,"externalStreamingInfo":{"contents":{"codec":{"video":"H265","audio":"AAC"},"format":"RAW","esInfo":{...}}}}}]}`
and `Feed` takes `{"bufferAddr":"0x..","bufferSize":n,"pts":ns,"esData":1 or 2}`.
S [UL: StarfishMediaAPIs.h:37-78], [KODI-M:645-758, 1140-1149], [SS4S-P:276-394].

API 2 (webOS 5 and later): `NDL_DirectMediaInit(appId)`,
`NDL_DirectMediaSetWindowId(id)`, `NDL_DirectMediaLoad(&info, callback)`,
`NDL_DirectVideoPlay(buffer, size, pts)`, `NDL_DirectAudioPlay(buffer, size, pts)`,
`NDL_DirectVideoSetArea(left, top, width, height)`,
`NDL_DirectMediaSetAppState(state)`, `NDL_DirectVideoSetHDRInfo(info)`,
`NDL_DirectMediaUnload()`, `NDL_DirectMediaQuit()`. API 1 (3.9 to 4.x) has
`NDL_DirectMediaInit(appId, releasedCallback)`, `NDL_DirectVideoOpen` and
`NDL_DirectAudioOpen` instead. S [UL: include/libndl-media/NDL_directmedia_v1.h:55-126,
NDL_directmedia_v2.h:23-171, NDL_directmedia_common.h:19-39]. ss4s builds
`ndl-esplayer` for 2.2 to 3.4, `ndl-webos4` for 3.5 to 4.x and `ndl-webos5`
from 5 (S [SS4S: modules/webos/ndl/]).

Both leave demuxing, most audio decoding, the clock, HLS, DASH, subtitles and
track switching to the app. Kodi brings FFmpeg for that.

## 2. Where the video is drawn

Video is on its own hardware plane behind the graphics plane. The app's surface
shows it where its pixels have alpha 0. Screenshots, `glReadPixels` and the
TV's capture service never contain the picture. S [FL: README.md], S-report
[NUV-R:349-351], [JWN-N], [PLX-U:244-245].

| Fact | Known |
|------|-------|
| The app needs `"transparent": true` in `appinfo.json` | S [FL: README.md]; plx-native sets it too [PLX: pkg/appinfo.json] |
| LG's plugin paints the video's rectangle with a transparent colour and `BlendMode.src` | S [FL: lib/src/hole.dart:43-61] |
| webOS 5 and later: the app asks the compositor for an "exported window" on its own `wl_surface`: `wl_webos_foreign.export_element(new_id, surface, 0)`. Type 0 is `video_object`, 1 `subtitle_object`, 2 `transparent_object`, 3 `opaque_object`. The id arrives in `wl_webos_exported.window_id_assigned(string, type)` | S [WEXT:22-95]; Kodi binds the global itself, "available since webOS 5.0" [KODI-W:60-61, 98-108] |
| The id reads `_Window_Id_<n>` | S [LSM-F:37-45]; S-report [JWN-N] (`_Window_Id_39`) |
| The id goes into the load payload as `option.windowId`. On 5 and later that string is the whole binding; a load without it fails with error 104 "fail to create AVProxy" | S-report [PLX-U:26, 40-49] (5.4.2); S [PLX: src/starfish.h:56-58] |
| Rectangle: `wl_webos_exported.set_exported_window(source_region, destination_region)`, both `wl_region`s. Version 2 adds `set_crop_region(original_input, source, destination)` and `set_property(name, value)` | S [WEXT:81-112] |
| The destination is in the app surface's own coordinates. The compositor multiplies by the surface item's scale, adds the surface's position, and sends the result to the video output service (`setDisplayWindow` or `setCropRegion` on `com.webos.service.videooutput`) | S [LSM-F:437-481, 749-799, 900-997], OSE's compositor. I that a TV does the same; S-report [NUV-E:1-16] agrees for a 3840x2160 surface (LG G5, 10.3.1), from user reports |
| The source region is in decoded-frame pixels, so a smaller source crops and zooms. ss4s and LG's plugin pass `0,0,videoWidth,videoHeight`; plx-native passes the canvas until `videoInfo` tells the size | S [SS4S: modules/webos/smp/src/smp_resource_webos5.c:72-82], [FL: lib/video_player_webos.dart:455-470], [PLX-U:173-176] |
| Properties the compositor reads: `z_index` (the exported item), `z_order` (the video plane), `fullscreen` (`"true"`, before `set_crop_region`), `mute`, `directVideoScalingMode` | S [LSM-F:1176-1260] |
| The exported item sits at z -1 under the app's surface content, and the compositor's punch-through item writes transparent pixels there with blending off. Anything the app draws opaque still covers the video | S [LSM-F:292-293, 1262-1274], [LSM: modules/weboscompositor/foreign/punchthroughelement.h:43-69] |
| The pipeline imports the window by id and attaches the punch-through; the app never does | S [WEXT:59-66, 148-165], [LSM-F:1595-1640] |
| SDL's wrappers: `const char *SDL_webOSCreateExportedWindow(SDL_webOSExportedWindowType)`, `SDL_bool SDL_webOSSetExportedWindow(const char *id, SDL_Rect *src, SDL_Rect *dst)`, `SDL_webOSDestroyExportedWindow(const char *id)`. The first waits up to one second for the id | S [SDL: include/SDL_system.h:634-658, src/video/wayland/SDL_waylandwebos_foreign.c:92-216]. SDL is not needed: the protocol works alone (S [KODI-W]; S-report [JWN-N]) |
| SDL 3, the fork's `webOS-3.4.x` line this package bundles: the same functions return `bool`, sit under `SDL_PLATFORM_WEBOS` (defined when `__WEBOS__` is, which the fork's CMake makes a public definition of its target when built with `-DWEBOS=ON`), and take `SDL_WEBOS_EXPORTED_WINDOW_TYPE_VIDEO` for a video plane. `Create` exports the surface of the current GL window and keeps the id in a 32-byte buffer until `Destroy`. `SetExportedWindow` with a null `src` or `dst` sends the whole window's rectangle, since "wl_webos_exported takes no null regions" | S [SDL3: include/SDL3/SDL_system.h:834-961, src/video/wayland/SDL_waylandwebos_foreign.c:34-43, 92-106, 155-259, CMakeLists.txt:2223] |
| Order used by the apps that ship: create the window, set its rectangle, then load with the id | S-report [NUV:2797-2807] |
| LG's plugin hides the hole while the rectangle moves and sets the exported rectangle again 100 ms after the last move | S [FL: lib/video_player_webos.dart:72, 100-135]. I: the plane does not follow the UI frame by frame |
| Graphics are 1920x1080 on UHD models and video is 3840x2160; `"resolution"` in `appinfo.json` can select 1280x720 | D [LG-RES]. Some models grant a 3840x2160 graphics surface (S-report [NUV-E:4-9]) |
| A hardware plane does not clip: on the ACB path a negative origin or a size beyond the panel blanks it | S-report [NUV-H:70-73] (C9). U on the exported window |
| webOS 4.x and earlier have no window in the media path. The plane is bound with `libAcbAPI`: `AcbAPI_create`, `AcbAPI_initialize(id, PLAYER_TYPE_MSE, appId, callback)`, `AcbAPI_setMediaId(id, mediaId)`, `AcbAPI_setSinkType(id, SINK_TYPE_MAIN)`, `AcbAPI_setState(id, APPSTATE_FOREGROUND, PLAYSTATE_LOADED, &task)`, `AcbAPI_setMediaVideoData(id, videoInfoJson, &task)`, `AcbAPI_setDisplayWindow(id, x, y, w, h, fullScreen, &task)` or `AcbAPI_setCustomDisplayWindow(id, sx, sy, sw, sh, dx, dy, dw, dh, fullScreen, &task)`, then `PLAYSTATE_PLAYING` | S [UL: include/appswitching-control-block/AcbAPI.h:9-74], [SS4S: modules/webos/smp/src/smp_resource_acb.c:14-136], [KODI-M:667, 1693-1751] |
| On 4.x the load still needs a non-empty `windowId` (any string): with an empty one it answers true, allocates a `mediaId` and never fetches | S-report [NUV:2675-2676] (C9) |

## 3. Formats

Everything in this section is D from LG's pages unless tagged. LG writes them
for web apps; the native pipeline behind `<video>` is the one section 1 loads
(I from [CHR] and [PLX-U:14-17]).

| Release | HLS | PlayReady | Widevine Modular | Codecs of note |
|---------|-----|-----------|------------------|----------------|
| 3.0 | v3 | yes | 2.2 | |
| 3.5 | v3 | 3.0 | 3.0.5, L1 from here | |
| 4.0 | v5 | 3.0 | 3.2.1 | |
| 4.5 | v5 | 3.0 | 14 | HEVC, and VP9 in MKV; no AV1 |
| 5.0 (2020) | v7 | 4.0 | 16 | AV1 (MP4, MKV), VP9 2160p; 8K models |
| 6.0 (2021), TV 22, TV 23 | v7 | 4.0, 4.4 from TV 22 | 16 | |
| TV 24, TV 25 | v7 | 4.4 | 17 | Opus in MKV listed from TV 24 |
| TV 26 | v7 | 4.6 | 17 | |

3.x is 2016 to 2017 and 4.x 2018 to 2019. Sources: [LG-SP], [LG-AV-45],
[LG-AV-50], [LG-AV-240], [LG-AV-250].

| Topic | Fact | Known |
|-------|------|-------|
| Streaming protocols LG lists | HTTP, HTTPS and HLS. HTTP/2 from 5.0, no HTTP/3 | D [LG-SP] |
| MPEG-DASH, Smooth Streaming | Not in LG's protocol table: LG points to MSE/EME, and says not to use Smooth Streaming. The transport types `MPEG-DASH` and `MSIIS` exist in LG's Flutter plugin and the OSE web engine | D [LG-SP], [LG-SS]; S [FL-P:274-279], [CHR:1027-1029]. U how well native DASH plays per release |
| HLS limits | AES-128 only. Live seek from 3.0. Fast forward, reverse and any speed but 1.0 are "not supported". Audio and video segments must have the same duration. Not supported: `EXT-X-PROGRAM-DATE-TIME`, `EXT-X-ALLOW-CACHE`, `EXT-X-DATERANGE`, `EXT-X-I-FRAMES-ONLY`, `EXT-X-I-FRAME-STREAM-INF`, `EXT-X-SESSION-DATA`, `EXT-X-INDEPENDENT-SEGMENTS`, `EXT-X-DISCONTINUITY-SEQUENCE`. `EXT-X-DISCONTINUITY` for PTS only, from 4.0. `EXT-X-MEDIA` from 4.0, never `TYPE=VIDEO`. v5 features (byte range, multi audio, WebVTT subtitles for VOD) "partially supported" | D [LG-SP] |
| HLS playlists | An audio-only variant beside video variants needs `CODECS`, with one codec | D [LG-HLS] |
| H.264 | BP/MP/HP, 1080p60 L4.2 at 40 Mbps on every model; 2160p30 L5.1 at 50 Mbps on UHD models, 2160p60 on some | D [LG-AV-250] |
| HEVC | Main and Main10, 1080p60 L4.1; 2160p60 L5.1 at 60 Mbps on UHD; 4320p60 L6.1 at 100 Mbps on 8K | D [LG-AV-250] |
| VP9, AV1 | 2160p60 at 50 Mbps on UHD models from 5.0; AV1 4320p60 on 8K | D [LG-AV-50], [LG-AV-250] |
| Containers | MP4, M4V, MOV (H.264, HEVC, AV1, MPEG-4), MKV (also MPEG-2, VP8, VP9), TS (H.264, HEVC, MPEG-2), AVI, 3GP, MPG, VOB, ASF/WMV on some models. At 2160p only H.264 or HEVC in MKV, MP4 or TS | D [LG-AV-250] |
| Audio in video files | AAC (not Main profile), Dolby Digital, Dolby Digital Plus, MP3, PCM; AC-4 and MPEG-H on some models; DTS in MKV through TV 22, then DTS, DTS-HD and DTS:X "on some models" from TV 23 | D [LG-AV-60], [LG-AV-230] |
| Subtitles LG documents | WebVTT `.vtt` | D [LG-AV-250] |
| Subtitles the pipeline has parsers for | ASS, TTML, WebVTT, plain text, DVB, DVB teletext, DXSA/DXSB, JCAP | S [UL: src/playerAPIs.lds] (`smp::SubtitleParser::parse*`), [UMS: uMediaserver.cpp:883] (`subtitleType`) |
| External subtitle file | `setSubtitleSource` takes a URL the pipeline downloads and syncs itself; the URL must end in a known extension (`.srt` worked, no extension gave error 210) | S-report [NUV:3880-3896] (C9) |
| Who draws subtitles | The pipeline, once `setSubtitleEnable` is true, with `setSubtitle{FontSize,Position,CharacterColor,BackgroundColor,...}` for style | S-report [NUV:3794-3864]. U where: the video plane or a `subtitle_object` window |
| HDR | LG's pages list no HDR format. `videoInfo` carries `hdrType`, SEI mastering data and VUI | D by absence [LG-AV-250]; S [UMS: include/public/uMediaClient.h:165-203] |
| Dolby Vision and Atmos by URI | Played on an LG G4 on 11.1.0 through `com.webos.media` | S-report [PLX-U:28] |
| One video at a time | Two `<video>` elements are "not officially supported" | D [LG-FAQ2] |
| Per-device limits | `/etc/umediaserver/device_codec_capability_config.json` on the TV | S-report [JWN-M] |

### DRM

| Fact | Known |
|------|-------|
| Web apps: `luna://com.webos.service.drm` `load` (DRM type, app id) gives a `clientId`; `sendDrmMessage` delivers the PlayReady initiator; `getRightsError` reports failures; the media loads with `option.drm = {"type":"playready","clientId":..}`. Widevine goes through MSE/EME | D [LG-DRM], [LG-MO] |
| Native, webOS 26: `option.drm = {"type":"PLAYREADY" or "WIDEVINE","licenseUrl":..,"useLicenseCallback":bool}` in the load payload. With the callback, the pipeline sends a challenge event and the app answers with `NDL_MediaSetDrmOperation(systemId, base64License, msgType, service, kid, sessionId, requestId)` | S [FLD-P:343-354, 596-626], [FLD: lib/src/drm_configs.dart] |
| `NDL_MediaDRM*` exist from 3.4.0 and mirror the web DRM service's calls | S [FW]; I from the names. No caller found |
| Whether `com.webos.service.drm` accepts a jailed native app, and DRM on releases 5 to 10 | U |

## 4. Events and properties

A `subscribe` event is a JSON object with one key naming it. `NDL_Media`'s
callback and `uMediaClient`'s raw mode deliver the same text. S [UMS:
uMediaserver.cpp:810-973, src/media_client/uMediaClient.cpp:202-684].

| Need | Event or call | Known |
|------|---------------|-------|
| Prepared | `{"loadCompleted":{"mediaId":M}}`. The web engine and LG's plugin call `play` from here | S [UMS: uMediaserver.cpp:830], [CHR:479-533] |
| Duration, seekable, track lists | `sourceInfo`: `container`, `seekable`, `programInfo[0].duration` (ms), `numAudioTracks`, `audioTrackInfo[]` (`language`, `codec`, `channels`, `sampleRate`, `bitRate`), `videoTrackInfo[]` (`codec`, `width`, `height`, `frameRate`, `bitRate`), `numSubtitleTracks`, `subtitleType`, `subtitleTrackInfo[]` (`language`) | S [UMS: uMediaserver.cpp:845-898], [FL-P:551-577] |
| `sourceInfo` shape | On OSE the duration and the track lists are inside `programInfo[0]`, beside `numVideoTracks`; `container`, `seekable` and `mediaId` are at the top. LG's plugin reads them from `programInfo[0]` too | S [UMS: uMediaserver.cpp:845-898], [FL-P:551-577] |
| `sourceInfo` gaps | The web engine treats HLS, Smooth Streaming and Widevine source info as insufficient; a set can leave out an audio track it cannot decode. The pipeline sends "a placeholder sourceInfo whose programInfo[0] has all-zero duration/track counts before the real event", and `videoInfo` "can land before sourceInfo" | S [CHR:1070-1076], [FL-P:562-566, 589-593]; S-report [NUV:1988] |
| Video size, HDR | `videoInfo`: `width`, `height`, `frameRate`, `bitRate`, `hdrType`, `SEI`, `VUI` | S [UMS: uMediaserver.cpp:911-949], [FL-P:578-599]. U whether it repeats on a rendition change |
| Position | `{"currentTime":{"currentTime":<ms>,"mediaId":M}}`. OSE's pipeline sends it on a 200 ms timer; "about every 200 ms" on a C9 | S [UMS: uMediaserver.cpp:834], [GMP: src/player/UriPlayer.cpp:32,79]; S-report [NUV:1003-1004] |
| Position, polled | `NDL_MediaGetCurrentPosition()`; `StarfishMediaAPIs::getCurrentPlaytime()` | S [FL-A:470], [UL: StarfishMediaAPIs.h:47] |
| Event rate | `uMediaClient::setUpdateInterval(int, int)` and `StarfishMediaAPIs::SetUpdateInterval(json)` exist; no luna method was found | S [FW], [UL: include/player-factory/pipeline.hpp:7]. U over luna |
| Buffered range | `bufferRange`: `beginTime`, `endTime`, `remainingTime`, `percent`. `endTime` is in seconds (I: the web engine compares it with the duration in seconds [CHR:638-641, 684-687]; a C9 logged 5, 11, 17, 23 [NUV:2589]); `remainingTime` -1 means fully loaded | S [UMS: uMediaserver.cpp:835], [CHR:627-655] |
| Buffering | `bufferingStart`, `bufferingEnd`. A source that dies mid-play sends `bufferingStart` and no error | S [UMS: uMediaserver.cpp:836-837]; S-report [NUV-H:120-126] |
| Playing, paused | `playing`, `paused`. Whether `paused` arrives on 5 and later is open at plx-native | S [UMS: uMediaClient.cpp:528-541]; S-report [PLX-U:141] |
| Seek | `seek` answers at once; `seekDone` follows on the subscription about 110 ms later. The web engine ignores `currentTime` while a seek is pending | S-report [PLX-U:163-171]; S [CHR:617-620] |
| End | `endOfStream`. `unloadCompleted` is not the end of the stream | S [UMS: uMediaserver.cpp:833]; S-report [JWN-N] |
| Track change | `selectTrack`, confirmed by `trackSelected` (`type`, `index`) or by an `audioInfo` naming the track. On a 4.5 set the default audio track's select is acknowledged with no event | S [UMS: uMediaserver.cpp:831]; S-report [PLX-U:146-159] |
| Rate | `setPlayRate`. The web engine keeps audio only at 1.0. LG documents other speeds as unsupported on streams | S [CHR:1143-1158]; D [LG-SP] |
| Volume | `setVolume` on `com.webos.media`; unsupported in LG's webOS 26 plugin | S [UMS: uMediaClient.cpp:1189-1200], [FL-P:485-493] |
| Error | `{"error":{"errorCode":n,"errorText":s,"mediaId":M}}` | S [UMS: uMediaserver.cpp:962-973] |
| Live | `seekableRanges` events when the load set `useSeekableRanges`; live seek is documented from 3.0 | S [UMS: uMediaClient.cpp:632-644], [CHR:1010-1011]; D [LG-SP] |

Error codes, S [FL: webos/webos_player.h:59-90] with the classes from
[CHR:825-900]: 100 playing error, 101 command not supported, 104 failed to
load; 200 audio codec not supported, 201 video codec not supported, 202 media
not found, 203 AV type not found, 204 failed to demultiplex, 210 unknown
subtitle; 300 network, 301 unsecured Wi-Fi; 400 to 403 HLS server answers 302,
404, 503, 504; 500 and 506 DRM; 600 resource manager, 601 resource allocation;
700 seek failure, 701 set play rate failure; 1000 pipeline died; 40000 and up
streaming protocol (a C9 printed `40403` for an HTTP 403, S-report [NUV-H:204-207]);
61441 buffer full and 61442 buffer low, which are not errors.

The in-process `StarfishMediaAPIs` callback is `(int type, long long number,
const char *text, void *context)` with `PF_EVENT_T` types: 0x03 duration, 0x04
video info, 0x07 audio info, 0x12 and 0x13 errors, 0x16 load completed, 0x19
seek done, 0x1a playing, 0x1b paused, 0x1c end of stream. S [UL:
StarfishMediaAPIs.h:88-124].

## 5. Resources, focus and the lifecycle

| Fact | Known |
|------|-------|
| Decoders belong to uMediaServer's resource manager. A pipeline acquires them at load; when another needs them, a policy action picks a victim by pipeline type priority, then background before foreground, then focus, then least recently used | S [UMS: src/resource_manager/ResourceManager.cpp:1230-1244], OSE |
| `notifyForeground` marks the connection as not selectable; `notifyBackground` makes it selectable | S [UMS: uMediaserver.cpp:1932-2013] |
| The TV's pipeline has the matching handlers (`policyActionHandler`, `policyUnloadCallback`) | S [UL: src/playerAPIs.lds]. U what the client then receives: an `error` in the 600 range, `unloadCompleted`, or nothing |
| Every client calls `notifyForeground` around the load: the web engine after `load`, Kodi and ss4s before `Load`, nuvio before `subscribe` | S [CHR:1160-1173], [KODI-M:755], [SS4S-P:77], [NUV:1649] |
| App hidden, web engine: `notifyBackground` on a TV build; on OSE's GStreamer build it unloads and reloads on resume with `playTime.start` at the last position | S [CHR:224-271] |
| App hidden, LG's Flutter plugin: unload every player and destroy its exported window; on resume create a new window and reload. Its `notifyVisibility` mutes the video output with `NDL_MediaMuteDisplay`, because "the platform media subsystem does not expose a direct foreground/background notification" | S [FL: lib/video_player_webos.dart:385-447], [FL-P:495-513] |
| The ES players must be told too: `NDL_DirectMediaSetAppState(FOREGROUND)`, or frames are dropped with no error | S-report [JWN-N] |
| ACB (4.x): `AcbAPI_setState(id, appState, playState, &task)` on every change, `PLAYSTATE_UNLOADED` on release | S [KODI-M:1724-1751] |
| A killed process leaves its bus name held for a while, so the next launch's fixed name is refused; a pid suffix avoids it | S-report [NUV:1869-1876] |
| Screensaver: `luna://com.webos.service.tvpower/power/registerScreenSaverRequest` with `{"subscribe":true,"clientName":appId}`, answered through `responseScreenSaverRequest` | S [NUV:1723-1731]. U whether a playing pipeline already holds it off |
| Audio focus between apps is not a separate API: the resource manager and the audio service decide. `focus` and `focusChanged` exist on OSE | S [UMS: com.webos.media.api.json:6, uMediaClient.cpp:624-631]. U on TVs |

## 6. What the simulator can substitute

Plain Linux has no uMediaServer, no video plane and no `wl_webos_foreign`. The
Tizen simulator makes `player_create` fail, and the same is the first step
here: the luna client cannot register and the player reports `error`. To run
the player's state machine, fake `com.webos.media`, since its contract is JSON
in and JSON events out: an in-process fake behind the same interface answers
`load` with a `mediaId`, then emits `sourceInfo`, `loadCompleted`, `playing`,
`currentTime` every 200 ms, `seekDone` and `endOfStream` from a clock. Real
pictures need GStreamer in the VM: OSE's URI pipeline is a `playbin` whose
state changes, position timer and `GST_MESSAGE_BUFFERING` map onto those
events (S [GMP: src/player/UriPlayer.cpp:340-476, 552]), with frames pulled
from an `appsink` into a texture drawn in the view's rectangle. All I.

What is built is simpler: the host's player talks to a backend interface, and
where no Luna bus answers the backend is `SimulatedPlayer`, a clock that plays
a made-up stream (a minute of 1080p, or what a `#sim:` fragment of the URI
asks for) and reports loading, position every 200 ms, buffering around a
seek, the end and injected errors. Its plane is a test picture composited
under the scene with `SkBlendMode::kDstOver`, so it shows only where the app
left its pixels transparent, as the TV's plane does. A fake `com.webos.media`
exists in the tests alone, where it replays the JSON of section 4 to
`LunaMediaPlayer`. Nothing decodes a picture.

## Open questions

Only a TV settles these.

1. Does a Developer Mode install get the role file on each release, and can
   it register `com.webos.media.client.<x>` and `load`? And a store install?
2. `mediaTransportType`: LG's plugin sends `HLS` beside `option` for `m3u8`
   URLs. nuvio saw a 4.10 set return a `mediaId` and never fetch when the
   field was sent [NUV-H:33-36], and sends it inside `option` in a later mode
   [NUV:2628-2635]. Which form each release needs is not known.
3. Is the destination rectangle in surface pixels on every release, also on a
   1280x720 or 3840x2160 surface?
4. Does the plane follow a moving view, and how often may the rectangle change?
5. Do `paused`, `seekDone` and `trackSelected` arrive on each release, and
   does `play` after `pause` resume or restart? [PLX-U:141-144]
6. Where does a live stream open, and what does `seek` do on it?
7. Does `setPlayRate` work on HLS (LG says no) and on files, with sound?
8. Do pipeline subtitles show in a native app, and where? Can the app get cue
   text instead (`subtitleData` on OSE, [UMS: uMediaClient.cpp:657-661])?
9. What does the client receive when another app takes the decoder, or when
   the app returns from the background?
10. Can any header beyond Referer, User-Agent and Cookie reach the pipeline?
    Nothing found says so.
11. `NDL_Media` before 11.x: the argument of `NDL_MediaInit`, the signatures
    of `NDL_MediaLoad` and `NDL_MediaSetDisplayWindow`, whether
    `NDL_Media_DL_Initialize` is required, whether init needs SDL.
12. Does MPEG-DASH play natively on each release, and DRM before webOS 26?
13. Can a jailed app build a GStreamer pipeline with LG's decoder and sink?
14. Does the panel switch to HDR10, HLG or Dolby Vision by itself on the URI
    path?

## What this means for react-native-webos

### The backend

**First choice: `com.webos.media` over luna-service2, with the exported
window.** LG's role template grants it, `uMediaClient` and `NDL_Media` do the
same underneath, nuvio ships on it, and it is C and JSON only:
`libluna-service2.so.3` and `libwayland-webos-client.so.1` are on every
firmware, so no C++ ABI and no missing symbol can stop the binary from
loading. The TV's own demuxers and decoders give HLS, files, tracks, subtitles
and seeking. It covers webOS 5 and later; 4.x would add the ACB binding
through `dlopen("libAcbAPI.so.1")`, a separate decision.

**Fallback: `libNDL_media.so.1` through `dlsym`,** probed at startup. LG ships
its own native apps on it, and it is the only known route to DRM (webOS 26).
It is second because no header is public, `SetPlayer` starts at 6.4.0,
`InitWithWindowId` at 11.2.0, and LG's comment says the older init wants SDL.

**Not planned: feeding `StarfishMediaAPIs` or `NDL_directmedia`.** That is
Kodi's design and puts FFmpeg, an audio path and a clock in the package. Its
one gain is full control of HTTP, which matters only if open question 10 ends
in "no" and an app cannot carry its token in the URL or a cookie.

### Play this HLS URL in this rectangle (webOS 5 and later)

1. At window creation: bind `wl_webos_foreign` from the registry, call
   `export_element(surface, 0)`, keep the id from `window_id_assigned`.
2. Once per process: `LSRegister("com.webos.media.client.<appId>.<pid>")` on
   a GLib context a player thread pumps. Every callback copies its JSON and
   posts to the main loop.
3. `set_exported_window(source: the whole surface, destination: the view's
   box in surface pixels)`, then commit the surface.
4. `load` with `{"uri":U,"type":"media","payload":{"mediaTransportType":"HLS","option":{"appId":A,"windowId":W,"useSeekableRanges":true,"bufferControl":{"userBufferCtrl":false},"transmission":{"httpHeader":{"userAgent":..,"cookies":..,"referer":..},"playTime":{"start":ms}}}}}`.
   Keep `mediaId` from the reply.
5. `notifyForeground {"connectionId":M}`, then `subscribe {"mediaId":M}`.
6. `sourceInfo` gives duration and tracks. `videoInfo` gives the size: set
   the window again, the frame as source, the `contentFit` rectangle as
   destination. `loadCompleted` means `readyToPlay`: `play` if the app asked.
7. Draw alpha 0 in the view's box while the player holds a `mediaId`.
8. `currentTime`, `bufferRange`, `bufferingStart`, `bufferingEnd`,
   `endOfStream` and `error` drive the observer the Tizen player drives.
9. Release: `unload`, then wait for `unloadCompleted` or a short timeout
   before the next load. Keep the registration and the exported window.
10. Hidden: `notifyBackground` and pause. If the pipeline is gone on return,
    reload at the last position.

### What the host's player calls become

The Tizen player's calls are in react-native-tizen's
`runtime/host/expo/modules/video/`, which [RNW] was ported from.

| Tizen call | webOS |
|------------|-------|
| `player_create`, `player_set_uri`, `player_prepare_async`, prepared callback | `load`, `subscribe`, `loadCompleted` |
| `player_set_display`, `_display_mode`, `_display_roi_area`, `_display_visible` | the exported window; alpha 0 hole; an empty destination or `unload` to hide |
| `player_set_video_roi_area` (`cover`) | the source region, in frame pixels |
| `player_start`, `player_pause`, `player_stop`, `player_unprepare`, `player_destroy` | `play`, `pause`, `unload` |
| `player_set_play_position` and its callback | `seek` and `seekDone`; no accurate flag per seek (`seekMode` is a load option) |
| `player_get_play_position`, `player_get_duration` | the last `currentTime`; `sourceInfo.programInfo[0].duration` |
| `player_get_state` | kept by the host from `playing`, `paused`, `loadCompleted` |
| buffering callback, `player_get_streaming_download_progress` | `bufferingStart`, `bufferingEnd`, `bufferRange` |
| completed, error, interrupted callbacks | `endOfStream`, `error`; the interrupt has no known event |
| `player_get_video_size`, `_video_stream_info`, `_codec_info`, video stream changed | `videoInfo`, `sourceInfo.videoTrackInfo` |
| `player_get_track_count`, `_track_language_code`, `_current_track`, `player_select_track` | `sourceInfo` track lists, `selectTrack`, `trackSelected` |
| `player_set_subtitle_updated_cb` and the host's `SubtitleOverlay` | `setSubtitleEnable`: the TV draws. The overlay is gone; a `subtitleData` event, if a TV sends one, is logged as an open question |
| `player_set_streaming_user_agent`, `_cookie` | `httpHeader.userAgent`, `.cookies`, plus `referer` |
| `player_set_playback_rate`, `_volume`, `_mute`, `_looping` | `setPlayRate`; `setVolume`, mute as volume 0; `seek` to 0 and `play` on `endOfStream` |
| `player_set_streaming_buffering_time`, `_max_adaptive_variant_limit` | `bufferControl` and `adaptiveStreaming.maxWidth`, `maxHeight`, both at load |

### What is built

In `runtime/host/expo/modules/video/` [RNW]. None of it has run on a TV.

| Part | File | Does |
|------|------|------|
| Backend interface | `PlayerBackend.h` | load, unload, play, pause, seek, rate, volume, track, foreground; reports through `PlayerBackendListener` |
| TV backend | `LunaMediaPlayer*.cpp`, `LunaMediaEvents.*`, `LunaMediaUnload.*` | the sequence above over a `LunaConnection`: `load`, `notifyForeground`, `subscribe`, then the events. A load waits for the `unloadCompleted` of the media before it, or 1.5 s |
| Bus | `platform/LunaClient.*`, `PlayerBackends.cpp` | registers `com.webos.media.client.<APPID>.<pid>` through luna-service2 on a thread of its own, once per process, never unregistered; falls back to the app's own `libhelpers` connection |
| Plane | `ExportedWindowPlane.*` | one exported window per player, made at its first load with the whole window as its rectangle, then placed at the view's rectangle on screen; the source is the `contentFit` crop in frame pixels once `videoInfo` or `sourceInfo` gave the size |
| Hole | `VideoView.*`, `VideoWindow.*` | the view clears its box with `SkBlendMode::kClear`, and the scene is cleared to transparent while a view shows a player. Fullscreen clears the whole window above every view |
| Player | `MediaPlayer*.cpp` | what a pipeline does not do: one seek in flight, play held until ready, looping by `seek` to 0 and `play` at `endOfStream`, progress ticks, a pause the pipeline makes by itself taken over |
| Open questions | `PipelineQuestions.*` | every guess below, in one file |

The package still has to write `"transparent": true` into `appinfo.json`
(section 2); without it the hole shows black.

What the code guesses, each logged at debug level as `[video n] open
question: ...` when playback meets it:

| Guess | Open question |
|-------|---------------|
| The bus name carries the app id and the process id; a refusal falls back to the app's own connection, which the TV may deny at `load` | 1 |
| `mediaTransportType: "HLS"` beside `option` when the URI contains `m3u8`, nothing for a file | 2 |
| `subscribe` carries `"subscribe": true` | none: OSE ignores it, plx-native sends it |
| The destination rectangle is the view's box in window pixels; the plane moves at every drawn frame in which the box moved | 3, 4 |
| `playing` and `paused` are waited for 1.5 s, `seekDone` 3 s, `trackSelected` 1 s after the call was accepted, then assumed | 5 |
| `pause` at `endOfStream`, so that a replay is `seek` then `play` from a paused pipeline | 5 |
| `setPlayRate` with `audioOutput: true` at every rate | 7 |
| Subtitles are the pipeline's: `selectTrack` then `setSubtitleEnable` | 8 |
| An `unloadCompleted` nobody asked for means the TV took the pipeline: the player fails with it | 9 |
| Headers other than User-Agent, Cookie and Referer are dropped with a warning | 10 |
| `bufferRange.endTime` is seconds | section 4 |
| `sourceInfo` after `loadCompleted` reports the media a second time; `loadCompleted` without one makes the player ready with no duration | section 4 |
| Errors 101, 210, 700, 701, 61441 and 61442 leave the media playing; every other one ends it and unloads | section 4 |
| `minBufferForPlayback` is not sent: `bufferControl` counts bytes | section 1.3 |
| Hiding the plane is the view no longer clearing its hole; no rectangle is sent for it | section 2 |

### Risks

- Every fact the design rests on comes from OSE source or from other people's
  TVs. Open questions 1 to 5 come first on hardware, recorded here with the
  set's model, year and release.
- `com.webos.media` is private. The same payloads appear in sources from 4.x
  to 11.x (I), and LG can still change them in any release.
- Three headers only: a stream that needs `Authorization` cannot play.
- The rectangle goes through the compositor and a service, so it lags the UI.
  A video view that moves needs its hole hidden meanwhile, as LG's plugin does.
- Rounded corners, transforms and opacity reach the hole, never the plane.
- A second player may take the first one's decoder, reported in an unknown way.
- Whether LG's store accepts a native app that calls `com.webos.media`
  directly is not known. LG's Flutter plugins reach the service through
  `libNDL_media`.

## Sources

| Tag | Source |
|-----|--------|
| LG-SP | LG, [Streaming Protocol and DRM](https://webostv.developer.lge.com/develop/specifications/streaming-protocol-drm) |
| LG-AV-nn | LG, AV formats per release: `https://webostv.developer.lge.com/develop/specifications/video-audio-<nn>` with 45, 50, 60, 220, 230, 240, [250](https://webostv.developer.lge.com/develop/specifications/video-audio-250), 260 |
| LG-MO | LG, [mediaOption Parameters](https://webostv.developer.lge.com/develop/guides/mediaoption-parameter) |
| LG-DRM | LG, [DRM Content Playback](https://webostv.developer.lge.com/develop/guides/drm-content-playback) |
| LG-RES | LG, [App Resolution](https://webostv.developer.lge.com/develop/specifications/app-resolution) |
| LG-HLS, LG-SS, LG-FAQ2 | LG FAQ: [HLS troubleshooting](https://webostv.developer.lge.com/faq/streaming-http-live-streaming-hls-troubleshooting), [Smooth Streaming](https://webostv.developer.lge.com/faq/streaming-can-i-develop-app-using-smooth-streaming), [two video tags](https://webostv.developer.lge.com/faq/can-i-use-two-video-tags-at-the-same-time) |
| UMS | [webosose/umediaserver](https://github.com/webosose/umediaserver/tree/af89f8ac3355027a90d7b299a78fb6fab9b6ffca) at `af89f8a` |
| GMP | [webosose/g-media-pipeline](https://github.com/webosose/g-media-pipeline/tree/df8470b2e0709f9a1f39bbe5e9fceb260bc20957) at `df8470b` |
| ROLE | [webosose/librolegen](https://github.com/webosose/librolegen/blob/b8fb817b533d80dcbcaacbb8af82a9c97f5c6fb5/templates/NativeApp.json) at `b8fb817` |
| CHR | webosose/chromium108 at `431ed1c`, [`src/media/neva/webos/umediaclient_impl.cc`](https://github.com/webosose/chromium108/blob/431ed1c647b24f2337a5883e8011a0c4da73e710/src/media/neva/webos/umediaclient_impl.cc) |
| LSM, LSM-F | [webosose/luna-surfacemanager](https://github.com/webosose/luna-surfacemanager/tree/b2fef6f73232244f6f13102aa66d2d3f5942835e) at `b2fef6f`; LSM-F is `modules/weboscompositor/foreign/webosforeign.cpp` |
| WEXT | webosose/webos-wayland-extensions at `4b24337`, [`protocol/webos-foreign.xml`](https://github.com/webosose/webos-wayland-extensions/blob/4b243378b451bd139466c242ae1e58753fea0fbf/protocol/webos-foreign.xml) |
| FL, FL-P, FL-A | [lg-flutter-webos/plugins](https://github.com/lg-flutter-webos/plugins/tree/3a742bb86b04e261044f0864f9b1f71acf0bbad3) at `3a742bb`, `packages/video_player`; FL-P is its `webos/webos_player.cc`; FL-A is `packages/audioplayers/webos/webos_player.cc` |
| FLD, FLD-P | the same repository, `packages/video_player_drm`; FLD-P is its `webos/webos_player.cc` |
| FW | [webosbrew/dev-toolbox-cli](https://github.com/webosbrew/dev-toolbox-cli/tree/2d5a7b6881850d825aa7fdc6c0b5e53c91dc40d3/common/data) at `2d5a7b6`, `common/data/<firmware>/<library>.json` |
| UL | [webosbrew/webos-userland](https://github.com/webosbrew/webos-userland/tree/00a5f8721e15d5c10030b99b2f3e73e0c527bcce) at `00a5f87`: headers and symbol lists written from the TV's libraries |
| SS4S, SS4S-P | [mariotaku/ss4s](https://github.com/mariotaku/ss4s/tree/dfba721b85420ccabf91dac65be73984bf1865f9) at `dfba721` (Moonlight's backend); SS4S-P is `modules/webos/smp/src/smp_player.c` |
| SDL | [webosbrew/SDL-webOS](https://github.com/webosbrew/SDL-webOS/tree/ba4c86af391d1524d3ff0cb61dc3b10035933239) at `ba4c86a`, branch `webOS-2.30.x` |
| KODI-M, KODI-W | [xbmc/xbmc](https://github.com/xbmc/xbmc/tree/383db99b78d5c81453fa3fd5783334b17d9ac984) at `383db99`: `xbmc/cores/VideoPlayer/MediaPipelineWebOS.cpp`, `xbmc/windowing/wayland/WinSystemWaylandWebOS.cpp` |
| PLX, PLX-U, PLX-P, PLX-D, PLX-C | [GLinnik21/plx-native](https://github.com/GLinnik21/plx-native/tree/f8f6dbb75c783de7c790608653e212a14c2ec66c) at `f8f6dbb`: `docs/uri-mode.md`, `rust-modules/play-uri/src/uri_protocol.rs`, `docs/distribution.md`, `docs/compatibility.md`. Its author's set is an LG 49SM9000PLA on 4.5; other sets are its users' reports |
| NUV, NUV-H, NUV-E, NUV-R | [iqui27/nuvio-native-legacy](https://github.com/iqui27/nuvio-native-legacy/tree/7cac4f25d11931b56c41cfefb952e4e183c45c1a) at `7cac4f2`: `src/video.c`, `src/video.h`, `src/video_escala.h`, `README.md`. Its author's set is an LG C9 on 4.10; other sets are its users' reports |
| JWN-N, JWN-M | [HookedBehemoth/jellyfin-webos-native](https://github.com/HookedBehemoth/jellyfin-webos-native/tree/c64e6ba0db) at `c64e6ba`: `docs/ndl.md`, `docs/multimedia.md` |
| SDL3 | [webosbrew/SDL-webOS](https://github.com/webosbrew/SDL-webOS/tree/release-3.4.16-webos.5) at tag `release-3.4.16-webos.5`, branch `webOS-3.4.x` |
| LS2H | [webosose/luna-service2](https://github.com/webosose/luna-service2/blob/5bc38aa348b35f5799f1bf9ed7c6cb513bc5f702/include/public/luna-service2/lunaservice.h) at `5bc38aa`, `include/public/luna-service2/lunaservice.h`; the openlgtv toolchain's sysroot ships the same declarations |
| GLIBH | `glib-2.0/glib/gmain.h` in the sysroot of the openlgtv toolchain, release 2026.08 |
| RNW | this repository: `runtime/host/expo/modules/video/`. The Tizen player it was ported from is react-native-tizen's directory of the same name |
