# Smart Hub Preview

The row of tiles Samsung's home bar shows above an app while the app is
highlighted, and what react-native-tizen can do with it. Checked on 2026-11-03.

| Tag | Meaning |
|---|---|
| **D** | Documented: a Samsung or Tizen page, or a TizenFX source file |
| **R** | Reported by a third party on their own TV (a GitHub issue), not by Samsung |
| **I** | Inferred from the rows above; not stated anywhere |
| **H** | Verified on hardware: the GQ75LS03DAUXZG (The Frame, 2024, Tizen 9.0), Developer mode, 2026-11-03 |
| **U** | Unknown |

## 1. What Samsung documents

Every page Samsung publishes on this feature is written for web apps
(`config.xml`, JavaScript). None mentions .NET, native apps, TPK packages or
`tizen-manifest.xml`. D by absence ([ov], [pub], [per], [api], [faq]).

| Fact | Tag | Source |
|---|---|---|
| The feature is "Smart Hub Preview": content shown "when the user hovers over your application icon in the launcher" | D | [faq] Q1 |
| Public preview: the same tiles for everyone, from "a remotely-hosted JSON file". Personal preview: tiles from "a JSON file managed by a background service application running on the TV" | D | [ov] |
| "All Samsung TVs from 2016 through 2021 support Smart Hub Preview", and the TV emulator since TV Extension 2.3.1. The FAQ says "supported since 2016 TV models". No page names 2022 or later | D | [ov], [faq] Q3 |
| TV and hospitality TVs; "not for signage display" | D | [cfg] |
| One preview per app; at least 1 section of at least 1 tile; at most 40 tiles | D | [api], [ov] |
| Tile image: PNG or JPG, at most 360 KB, shown 250 px tall; ratios 16:9 (444x250 to 640x360), 4:3 (333x250 to 480x360), 1:1 (250x250 to 360x360), 2:3 (167x250 to 240x360) | D | [ov] |
| No length limit for titles, "long titles or subtitles can cause unintended Smart Hub Preview behavior"; titles show only on the focused tile | D | [faq] Q6, Q7 |
| "URL must be unique" for each deep link | D | [ov] |
| No third-party advertising and no advertising of app features in tiles | D | [ov] |
| No privilege, partner status or Seller Office step is named on any preview page | D by absence | [ov], [pub], [per], [api], [faq] |
| An app installed from a USB drive on a 2017 or later TV gets no preview: "a known limitation" | D | [faq] Q11 |

### The JSON

Top level `sections` (mandatory); public preview only: `expires` (UNIX time, at
most one week ahead) and `expires_only` (update only at `expires`, 2018 models
and newer). A section: `title`, `position` (ascending order), `tiles`
(mandatory). A tile: `title`, `subtitle`, `image_url` (mandatory),
`image_ratio` (`16by9`, `4by3`, `1by1` default, `2by3`), `action_data` (the
string the app receives), `is_playable` (a Play icon), `display_from`,
`display_until` (UNIX time), `position`; `title_display_mode` and
`subtitle_display_mode` take `AlwaysOn`, `HighlightedOnly` (default),
`AccessibilityOnly`. D [pub]. The API page marks `image_ratio`, `action_data`
and `is_playable` mandatory, yet its own example omits the first two and
writes `action_url`. D [api]

Samsung's sample, cut to one tile ([json]):

```json
{
  "expires": 4106938892,
  "sections": [{
    "title": "Popular VOD",
    "tiles": [{
      "title": "Funny",
      "subtitle": "Birthday Party",
      "image_ratio": "16by9",
      "image_url": "http://developer.samsung.com/onlinedocs/tv/Preview/1.jpg",
      "action_data": "{\"videoIdx\": 1}",
      "is_playable": true
    }]
  }]
}
```

### Refresh

| Fact | Tag | Source |
|---|---|---|
| Public preview is fetched when the app is added to the launcher, when the TV is switched on after a full power-off, every 10 minutes, and at `expires`; "the minimum time between updates is 10 minutes" | D | [faq] Q9, Q12, [pub] |
| Each fetch sends `ACCEPT-LANGUAGE`, `X-SAMSUNG-APP-VERSION` and `X-SAMSUNG-COUNTRY-CODE`; nothing identifies the TV or the user | D | [pub] Table 2 |
| The JSON may be served as `text/plain` or `application/json` | D | [faq] Q8 |
| Personal preview: "service application is automatically launched by system every 10 mins regular basis"; the next `setPreviewData` "should be called at least 10mins later" | D | [per] |
| The service must call `exit()` in the success callback of `setPreviewData`; otherwise "it is automatically terminated after 5 minutes" and is not relaunched at the next interval | D | [per], [faq] Q12 |
| Check `http://tizen.org/feature/web.service` before launching a service; without it the foreground app misbehaves | D | [faq] Q13 |

### Web app setup, the only one documented

```xml
<!-- public -->
<tizen:metadata key="http://samsung.com/tv/metadata/use.preview"
                value="endpoint_URL=http://yourServer/JSONfile.json"/>
<!-- personal -->
<tizen:metadata key="http://samsung.com/tv/metadata/use.preview" value="bg_service"/>
<tizen:service id="g3rAasdgsM.service" auto-restart="false">
  <tizen:content src="service/service.js"/>
  <tizen:name>service</tizen:name>
  <tizen:category name="http://tizen.org/category/service"/>
</tizen:service>
<!-- both: do not reload the page on an app control -->
<tizen:app-control>
  <tizen:src name="index.html" reload="disable"/>
  <tizen:operation name="http://samsung.com/appcontrol/operation/eden_resume"/>
</tizen:app-control>
```

D [pub], [per]. The service id is `[application ID].[service name]`; the
foreground app launches it with `tizen.application.launchAppControl` and they
may talk over the Message Port API, port names left to the app. D [per]

### A tile press

| Fact | Tag | Source |
|---|---|---|
| The tile's `action_data` reaches the app as app-control extra data under the key `PAYLOAD`, a string array whose first value is JSON with a `values` field holding the `action_data` string: `{"key": "PAYLOAD", "value": ["{\"values\": \"{\\\"videoIdx\\\": 1}\"}"]}` | D | [pub] |
| Running app: an `appcontrol` event. Starting app: `getRequestedAppControl()` at load | D | [pub] |
| `setActionDataEventListener` is deprecated since 4.0 | D | [api] |
| Entering through a tile changes Return: on a detail page, Return goes to the previous page; on the home page it "must terminate the application without any confirmation popup", back to Smart Hub Preview | D | [ov] |
| For a .NET app an app control arrives in `OnAppControlReceived`, on first launch and on each later one; extras are `ReceivedAppControl.ExtraData` | D | [td-appctl] |
| `ExtraData.TryGet(key, out string)` and `TryGet(key, out IEnumerable<string>)`, with `IsCollection(key)` telling which | D | [tfx-appctl] lines 1446, 1488, 1589 |

The Return rule above contradicts the exit popup Samsung's general guide asks
for on a home page ([sam-term] in [tizen-tv-platform.md](tizen-tv-platform.md),
section 4): a launch from a tile needs its own flag.

## 2. What other developers saw

| Fact | Tag | Source |
|---|---|---|
| A web app's personal preview works on a Tizen 5.5 TV | R | [jf-318], 2025-10-22 |
| On Tizen 9: nothing on an S95C (2024) and on an S95D at first; later "working fine" on the S95D after a fresh install from VS Code, and "working on my Q60D from 2025 with Tizen 9" | R | [jf-318], 2026-01-27 to 2026-04-14 |
| No preview row on a 2024 set running Tizen 9 (model not named) | R | [cr-980] |
| The Tizen 9 emulator shows the preview but "does not host the Smart Hub Preview service as the real TV does" | R | [jf-318], 2026-02-10 |
| Apps installed in Developer mode over sdb (VS Code, Tizen Studio) get previews; whether a store install is needed is asked and unanswered | R | [jf-318] |

## 3. What a .NET TPK can use

On the TV:

| Fact | Tag |
|---|---|
| `ContentDBAPI.SetPreviewMetadata(json, packageId)`, loaded by reflection from the TV's framework (assembly 4.4.0.0) in a TPK holding `appmanager.launch`, returns `true` for Samsung's JSON with `package_name` added; for the example app and for KROMA | H |
| `MessagePort.Send` of `{DATA_COMMAND: SET_PREVIEW_DATA, DATA: json}` from a local port `EDEN_WEBAPIS_PORT` to `com.samsung.tv.contentservicemanager`, port `PARTNER_REQUEST_PORT`, succeeds (the port exists); nothing answered within 8 s | H |
| A `service-application` in the same TPK (`exec` its own DLL, `type` `dotnet`) starts on an app control the UI app sends at `OnPause`, runs in its own process and exits | H |
| Whether either channel makes Smart Hub show the tiles, and what a tile press delivers | U: needs someone looking at the home bar (Verify, steps 1 to 5) |

| Path | Status for a TPK | Partner |
|---|---|---|
| Public preview: the `use.preview` metadata with `endpoint_URL=<url>` on the `ui-application` in `tizen-manifest.xml` | U. Documented for `config.xml` only; a TPK's manifest carries the same `metadata` element ([td-manifest]) | none documented |
| Personal preview through `webapis.preview` | Not available: it is a Samsung Product API for web apps (D, [api]) | |
| Message port `SET_PREVIEW_DATA` to `com.samsung.tv.contentservicemanager` | U. Undocumented; the send succeeds on the TV (H, above), and whether tiles follow is not known | U |
| `ContentDBAPI.SetPreviewMetadata` resolved at run time from the TV's framework | U. Undocumented; Samsung forbids shipping TizenFX TV assemblies in a package ([sam-netfaq] Q4), so it can only be loaded from the TV, where it returns `true` (H, above) | U |

The public preview cannot be personal: its URL is fixed in the manifest and
the request carries only language, country and app version (section 1). A
client of a self-hosted server, like KROMA, needs the personal path. I

A store package with react-native-tizen already needs a Seller Office partner
group for its native `.so` ([tizen-tv-platform.md](tizen-tv-platform.md),
section 1.2). The preview adds no documented requirement; whether Samsung's
review accepts an undocumented channel is a question for the Seller Office content
manager. I

## 4. Design for an Expo app

Built, with the differences the table names; the developer's guide is
[`../tv-features.md`](../tv-features.md#smart-hub-preview).

| Piece | Where | As built |
|---|---|---|
| Configuration | `src/config/tv-features.ts`, `src/package/feature-manifest.ts` | as below. The service app is its own project, `PreviewService/`, referenced by the shell's: Tizen.NET.Sdk 1.2.1 packs a referenced Tizen project's DLL and merges its `tizen-manifest.xml` into the package's (`MergeManifest` in `build/Tizen.NET.Sdk.Packaging.targets`, D; the merged manifest seen in a TPK this package built) |
| Publishing from the app | `shell/PreviewPublisher.cs`, called by the host through `rn_tizen_host_set_preview_publisher` | tries `ContentDBAPI.SetPreviewMetadata` loaded from the TV's framework, then the message port; the JSON gains `package_name`, as in the H rows of section 3. The app publishes itself instead of starting the service for each `set` |
| JavaScript | `src/api/preview.ts`, `src/api/preview-json.ts`, `src/api/publish-queue.ts` | `set` resolves with `{ published, channel }` or `{ published: false, reason }` rather than rejecting; `isAvailable` and `launchedFromTile` added; `toSamsungPreview` lives in `@kromatv/react-native-tizen/preview-json`, which imports no React Native |
| Service app | `shell/preview-service/` | runs on Smart Hub's launch (U) and on an app control the app sends at `OnPause`; fetches `setSource`'s URL when `refreshMinutes` have passed, then publishes through the same channels and exits |
| Deep link | `shell/TilePayload.cs`, `shell/ReactNativeApplication.cs` | as below; every app control also tells the host whether it came from a tile, which `SmartHubPreview.launchedFromTile` answers |
| Back after a tile launch | the app | the host cannot know which screen is the first one, so it exposes `launchedFromTile` and the app skips its exit question |

The design it follows:

### Configuration

`expo.tizen.preview` takes one of two shapes:

```json
{ "expo": { "tizen": { "preview": { "endpoint": "https://example.com/tv/preview.json" } } } }
```

```json
{ "expo": { "tizen": { "preview": { "refreshMinutes": 30 } } } }
```

`rn-tizen package` turns them into the manifest. `endpoint` adds one line to
the `ui-application`:

```xml
<metadata key="http://samsung.com/tv/metadata/use.preview" value="endpoint_URL=https://example.com/tv/preview.json" />
```

`refreshMinutes` (10 or more, Samsung's minimum) adds the personal setup: the
`bg_service` metadata, the app control the web setup declares, a second
application built from a template beside `shell/`, and the privilege that lets
the UI app start it:

```xml
<ui-application appid="{{package}}" ...>
  <metadata key="http://samsung.com/tv/metadata/use.preview" value="bg_service" />
  <app-control>
    <operation name="http://samsung.com/appcontrol/operation/eden_resume" />
  </app-control>
</ui-application>
<service-application appid="{{package}}.preview" exec="PreviewService.dll" type="dotnet"
                     nodisplay="true" taskmanage="false" multiple="false" auto-restart="false">
  <label>{{label}} preview</label>
</service-application>
<privileges>
  <privilege>http://tizen.org/privilege/internet</privilege>
  <privilege>http://tizen.org/privilege/appmanager.launch</privilege>
</privileges>
```

`service-application`, its attributes and the `dotnet` type are the manifest
schema's ([td-manifest]); `appmanager.launch` is Public (tizen-tv-platform.md,
section 3). D. Whether Smart Hub reads `bg_service` from a TPK and launches its
service app every 10 minutes: U.

### JavaScript API

```ts
import { SmartHubPreview } from '@kromatv/react-native-tizen/preview';

export type TileRatio = '16:9' | '4:3' | '1:1' | '2:3';

export interface PreviewTile {
  readonly title?: string;
  readonly subtitle?: string;
  /** PNG or JPEG, at most 360 KB, 250 to 360 px tall in the ratio's range. */
  readonly image: string;
  /** Samsung's default is 1:1. */
  readonly ratio?: TileRatio;
  /** Delivered to the app through `Linking`; unique across the preview. */
  readonly url: string;
  readonly playable?: boolean;
  readonly showFrom?: Date;
  readonly showUntil?: Date;
}

export interface PreviewSection {
  readonly title?: string;
  readonly tiles: readonly PreviewTile[];
}

export interface PreviewSource {
  /** Returns Samsung's preview JSON; fetched by the service while the app is closed. */
  readonly url: string;
  readonly headers?: Readonly<Record<string, string>>;
}

export declare const SmartHubPreview: {
  /** Publishes now, or at the next allowed time: Samsung takes one update per 10 minutes. */
  set(sections: readonly PreviewSection[]): Promise<void>;
  clear(): Promise<void>;
  /** Where the service fetches tiles while the app is closed; null stops fetching. */
  setSource(source: PreviewSource | null): Promise<void>;
};
```

`set` validates with zod before anything leaves JavaScript: 1 to 40 tiles, at
least one per section, unique `url`s, `http` or `https` images. It writes
Samsung's JSON (`image_ratio` `16by9`..., `action_data` =
`{"url":"<url>"}`, `display_from` in UNIX seconds), stores it in the package's
data directory, and starts the service app with an app control asking it to
publish. Calls inside 10 minutes coalesce: the last one wins. A Node helper,
`toSamsungPreview(sections)`, gives a server the same JSON for `setSource`.

### Refreshing while the app is closed

React Native cannot run there: no Hermes, no window, and a background app gets
limited CPU (tizen-tv-platform.md, section 3). The service app is a small C#
`ServiceApplication` with no JavaScript:

1. On any app control: read `preview-source.json` (URL and headers) from the
   package's data directory; with none, republish the last `preview.json`.
2. Fetch with a 10-second timeout and a size cap, parse with `System.Text.Json`,
   keep the last good file on any failure.
3. Publish through the first channel that answers on the TV (section 3, in the
   order public endpoint, message port, `ContentDBAPI`), then `Exit()`, well
   inside Samsung's 5 minutes.

It starts when Smart Hub launches it (U for a TPK), when the UI app calls
`set`, and when the UI app goes to the background (`OnPause`). Both apps
reading one data directory rests on Tizen giving a package one data
directory: I, verify.

### Deep link

`ReactNativeApplication.OnAppControlReceived` already passes
`ReceivedAppControl.Uri` to `rn_tizen_host_set_initial_url` at launch and to
`rn_tizen_host_open_url` later. It gains one step before that: when the extras
hold `PAYLOAD`, take its first value (`TryGet` as a string, or the first of the
collection), parse `values` from it, parse that as `{"url": ...}`, and use the
URL as if it were the app control's `Uri`; an `action_data` that is already a
URL passes through. `Linking.getInitialURL()` then returns it on a cold start
and the `url` event fires while the app runs, so a tile routes like any other
deep link. The host also records that the launch came from a tile, so the root
screen's Back exits without the popup (section 1).

## Verify on a TV

On the 2024 Samsung The Frame (LS03D), Tizen 9.0, in Developer mode:

1. A TPK with `use.preview` = `endpoint_URL=https://...` and no service: do
   tiles show, and when is the URL fetched (server log)? Partly answered: with
   `endpoint_URL=http://<LAN host>/preview.json`, installed over sdb, the TV
   made no request in the 14 minutes after installing (H). Whether it fetches
   once the app is in the home bar, or after a full power cycle: U.
2. The same TPK with `bg_service` and a service application: does Smart Hub
   launch the service every 10 minutes (have the service report to `rn-tizen`
   over the log stream)? The service logs every start with its caller; only
   starts by the app itself were seen so far (section 3, H).
3. The message port: from the UI app, `message_port_check_remote_port` for
   `PARTNER_REQUEST_PORT` on `com.samsung.tv.contentservicemanager`, then send
   `SET_PREVIEW_DATA`; record the answer. Answered: the send succeeds, no
   answer (section 3, H).
4. `Type.GetType("Tizen.TV.Service.ContentService.ContentDBAPI, Tizen.TV.Service.ContentService")`
   from the shell: present, and does `SetPreviewMetadata` throw? Answered:
   present, returns `true` (section 3, H). Whether the tiles then show is step 5.
5. Press a tile with the app closed and with it running: the operation, `Uri`
   and every `ExtraData` key, `PAYLOAD` as string or collection.
6. Read the result into this file with the TV, year and Tizen version, as H.

## Sources

[ov]: https://developer.samsung.com/smarttv/develop/guides/smart-hub-preview/smart-hub-preview.html
[pub]: https://developer.samsung.com/smarttv/develop/guides/smart-hub-preview/implementing-public-preview.html
[per]: https://developer.samsung.com/smarttv/develop/guides/smart-hub-preview/implementing-personal-preview.html
[api]: https://developer.samsung.com/smarttv/develop/api-references/samsung-product-api-references/preview-api.html
[faq]: https://developer.samsung.com/smarttv/develop/faq/smart-hub-preview.html
[cfg]: https://developer.samsung.com/smarttv/develop/guides/fundamentals/configuring-tv-applications.html
[json]: https://developer.samsung.com/onlinedocs/tv/Preview/sampleJSON.json
[sam-netfaq]: https://developer.samsung.com/smarttv/develop/faq/tizen-net-tv.html
[sam-term]: https://developer.samsung.com/smarttv/develop/guides/fundamentals/terminating-applications.html
[td-manifest]: https://github.com/Samsung/tizen-docs/blob/master/docs/application/native/reference/manifest-xml.md
[td-appctl]: https://github.com/Samsung/tizen-docs/blob/master/docs/application/dotnet/guides/exchange-data-between-apps/app-controls.md
[tfx-appctl]: https://github.com/Samsung/TizenFX/blob/main/src/Tizen.Applications.Common/Tizen.Applications/AppControl.cs
[jf-318]: https://github.com/jellyfin/jellyfin-tizen/issues/318
[cr-980]: https://github.com/dougiefresh49/comic-reader/issues/980
