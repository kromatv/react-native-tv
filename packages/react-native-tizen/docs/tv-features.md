# TV features

Four Samsung TV integrations an app turns on in `app.json`. Each is off until
asked for, and the same `app.json` and code still build for Android, iOS and
the web: the keys are props of the `@kromatv/react-native-tizen` config plugin, and the JavaScript API answers
"not available" everywhere but on a Samsung TV.

| Feature | Turn it on | Import |
|---|---|---|
| [Voice](#voice) | `voice` prop | `@kromatv/react-native-tizen/voice` |
| [Smart Hub preview](#smart-hub-preview) | `preview` prop | `@kromatv/react-native-tizen/preview` |
| [Background audio](#background-audio) | expo-video's config plugin | nothing new |
| [4K UI](#4k-ui) | `resolution` prop | nothing new |

Picture-in-picture over other apps does not exist on Samsung TVs; see
[the reference](reference/tizen-tv-app-integration.md#1-picture-in-picture).
An in-app mini player is a small `VideoView` and needs nothing.

The facts behind each choice, with sources, are in
[`reference/tizen-tv-app-integration.md`](reference/tizen-tv-app-integration.md),
[`reference/tizen-smart-hub-preview.md`](reference/tizen-smart-hub-preview.md)
and [`reference/tizen-tv-platform.md`](reference/tizen-tv-platform.md), section 7.

## Voice

Samsung's voice assistant (the remote's microphone button) talks to the app
while it is in front: transport commands, "select" a title on screen, and
search.

```json
{ "expo": { "plugins": [["@kromatv/react-native-tizen", { "package": "com.example.myapp", "voice": { "searchUrl": "myapp://search?q={query}" } }]] } }
```

`"voice": true` turns it on with no search URL. `rn-tizen package` adds the
privileges `http://developer.samsung.com/privilege/voicecontrol` and
`http://developer.samsung.com/privilege/microphone`, both Public.

What works with no code:

| The user says | What happens |
|---|---|
| "pause", "play", "stop", "restart" | the playing expo-video player obeys |
| "skip forward 30 seconds", "go back 10 seconds" | `seekBy(30)`, `seekBy(-10)` |
| "go to one hour twenty" | `currentTime = 4800` |
| "subtitles on", "subtitles off" | the first subtitle track, or none |
| "select Dune" | the view on screen whose `accessibilityLabel` is "Dune" is pressed, as a tap would: its `onPress` runs |
| "select the second one" | the second labelled view, in reading order |
| "search Blade Runner" | `searchUrl` with `{query}` filled is opened through `Linking`, so the app routes it like any deep link |
| "up", "down", "next page" | the TV sends arrow keys, which the app already handles |

`searchUrl` takes `{query}` (the title, else the whole utterance),
`{utterance}`, `{title}`, `{genre}`, `{cast}` and `{contentType}`, each
percent-encoded; any other placeholder is refused at build time.

To take over, listen:

```ts
import { Voice } from '@kromatv/react-native-tizen/voice';

Voice.isAvailable; // false off a TV, in the simulator, and without the voice prop

const search = Voice.addListener('search', (query) => {
  // { utterance: 'search blade runner', title: 'blade runner', genre?, cast?, ... }
  router.push(`/search?q=${encodeURIComponent(query.title ?? query.utterance)}`);
});

const commands = Voice.addListener('command', (command) => {
  // { type: 'play' } | { type: 'skip', seconds: -10 } | { type: 'seek', position: 4800 }
  // | { type: 'subtitles', enabled: true } | next, previous, fastForward, rewind...
});

Voice.setScreen('player'); // home, list, details, player, search, settings; null for the default
```

While a `search` listener is registered the search URL is not opened; while a
`command` listener is registered the player is not driven. The logs name each
command; what was said, a search or a title, shows only at `debug`
(`rn-tizen run --level debug`). `setScreen` tells
the assistant what is on screen before each utterance; the default is
`player` while a video plays and `list` otherwise.

Store review: Samsung documents voice only for web apps
(`webapis.voiceinteraction`), and a .NET app has no published voice API. The
shell speaks Samsung's voice service protocol directly with public TizenFX
APIs, which keeps Tizen.NUI out of the process. Both the protocol and the
service are unpublished. Whether Samsung's review accepts an
app using them is a question for your Seller Office content manager; ask
before shipping voice to the store.

In the simulator `Voice.isAvailable` is `false`: there is no Samsung voice
service to register with.

## Smart Hub preview

The row of tiles Samsung's home bar shows above the app while its icon is
highlighted. Pressing a tile opens the app with the tile's URL, through
`Linking`, on a cold start and while it runs.

Two kinds. A **public** preview is the same for everyone, from a JSON file
Smart Hub fetches itself:

```json
{ "expo": { "plugins": [["@kromatv/react-native-tizen", { "package": "com.example.myapp", "preview": { "endpoint": "https://example.com/tv/preview.json" } }]] } }
```

A **personal** preview is published by the app, and refreshed while it is
closed by a small service app the package then carries:

```json
{ "expo": { "plugins": [["@kromatv/react-native-tizen", { "package": "com.example.myapp", "preview": { "refreshMinutes": 30 } }]] } }
```

`refreshMinutes` is 10 or more, Samsung's minimum. It adds the privilege
`http://tizen.org/privilege/appmanager.launch`, Public.

```ts
import { SmartHubPreview } from '@kromatv/react-native-tizen/preview';

const result = await SmartHubPreview.set([
  {
    title: 'Continue watching',
    tiles: [
      {
        title: 'Blade Runner',
        subtitle: '1 h 12 left',
        image: 'https://example.com/posters/blade-runner-640x360.jpg',
        ratio: '16:9',
        url: 'myapp://item/42',
        playable: true,
      },
    ],
  },
]);
// { published: true, channel: 'ContentDBAPI' } or { published: false, reason }

SmartHubPreview.setSource({ url: 'https://example.com/me/preview.json', headers: { Authorization: 'Bearer …' } });
SmartHubPreview.launchedFromTile; // exit on Back at the first screen, with no popup
```

`set` checks Samsung's rules before anything leaves JavaScript and throws a
`PreviewValidationError` naming each broken one: 1 to 40 tiles, at least one
per section, a unique `url` per tile, an http or https `image`, `showFrom`
before `showUntil`. Samsung also asks for PNG or JPEG images of at most 360 KB,
250 to 360 px tall (16:9 is 444x250 to 640x360); `set` cannot check those.
Samsung takes one update per 10 minutes: a call inside that window waits, and
the last one wins.

`setSource` names where the service app fetches tiles while the app is
closed: a URL returning Samsung's preview JSON, which a server writes with
`toSamsungPreview` from `@kromatv/react-native-tizen/preview-json` (no React
Native import, so Node runs it). A source with `headers` must be `https`,
since headers usually carry a credential: `setSource` throws otherwise. The
service fetches when the app goes to the background and when Samsung starts
it, at most once per `refreshMinutes`.

Samsung asks that Back on the first screen exit with no confirmation after a
tile launch, though its general guide asks for a confirmation otherwise:

```ts
BackHandler.addEventListener('hardwareBackPress', () => {
  if (SmartHubPreview.launchedFromTile) return false; // exits
  confirmExit();
  return true;
});
```

Treat what a tile delivers as untrusted input. Another app on the TV can send
yours the app control a tile press sends, with a `PAYLOAD` of its choosing, so
the URL `Linking` hands you and `launchedFromTile` say what the launch claims,
not that Samsung's home bar made it. Route the URL like any deep link: accept
your own scheme and the paths you expect, and never let it skip sign-in or a
confirmation, or name a file or a server the app then loads.

Store review: Samsung documents the personal preview for web apps only. A .NET
app publishes through `ContentDBAPI`, an unpublished class in the TV's
framework, or through an undocumented message port of Samsung's content
service. Ask your Seller Office content manager before shipping.

## Background audio

expo-video's own config plugin option turns it on:

```json
{ "expo": { "plugins": [["expo-video", { "supportsBackgroundPlayback": true }]] } }
```

`rn-tizen package` then declares `<background-category value="media" />`, so
Tizen may keep the app running while hidden. A player keeps playing while the
app is hidden only if its `staysActiveInBackground` is `true`; every other
player pauses when the app leaves the foreground, as expo-video does on
Android, and nothing resumes by itself. Video is never shown while hidden,
and nothing plays once the app is closed.

Samsung's guides say a hidden app pauses; whether a TV honours the category
is in [the reference](reference/tizen-tv-app-integration.md#2-playback-while-hidden-or-closed).
`supportsPictureInPicture` is ignored on Tizen.

## 4K UI

TV apps lay out for 1920x1080 points. On a 4K panel the window manager
normally gives the app a 1920x1080 window and scales it up, so text and
lines are upscaled. An app can ask for the full panel:

```json
{ "expo": { "plugins": [["@kromatv/react-native-tizen", { "package": "com.example.myapp", "resolution": "4k" }]] } }
```

`rn-tizen package` writes
`<metadata key="http://tizen.org/metadata/app_ui_type/base_screen_resolution" value="4k_only" />`,
and the host sizes the window to the screen the window manager reports. On a
3840x2160 screen the app still lays out 1920x1080 points, at a point scale of
2: text, borders and images are drawn at twice the pixels. On any other
screen nothing changes. The log says what the TV gave:

```
Screen: system_info 1920x1080, elm_win_screen_size_get 3840x2160, ...; window 3840x2160, canvas 3840x2160 (asked for a 4K UI)
```

The cost: a 3840x2160 window buffer is four times the memory and every full
redraw shades four times the pixels; Samsung's memory budget for a UHD app is
380 MB. Use it when the UI's sharpness matters more than its frame rate.

Not every 4K TV grants it. The 2024 Samsung The Frame (LS03D), Tizen 9.0, reports
1920x1080 from every source with the metadata and refuses a larger window, so
there the app stays at 1080p; the log line says so.

## Try them

`example/src/screens/platform/` shows each feature:

```sh
cd example
bun ../src/cli/main.ts run --device <tv-ip> --entry src/screens/platform/main.ts
```

Left and Right switch screens. Voice: say "select Dune", "pause", "search
Blade Runner"; Play/Pause moves transport commands between the player and a
JavaScript listener. Smart Hub preview: Enter publishes four tiles; press one
from the home screen. Background audio: Enter toggles `staysActiveInBackground`,
then press Home and come back. Screen and 4K: the point scale and fine lines.
