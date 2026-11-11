# Remote control of an LG webOS TV over the network (SSAP)

How `rn-webos` can press remote keys on an LG TV, launch and close an app, type
into the on-screen keyboard and ask for a screenshot: the protocol LG's phone
apps use, which the libraries call SSAP (URIs start with `ssap://`; the TV
advertises it as `webos-second-screen`). LG publishes almost nothing about it,
so nearly every fact here is read from the source of clients that ship, or
reported by their users. Researched on 2026-11-11 from documentation and source
only: no TV was contacted, and nothing below is verified on hardware by this
project.

| Tag | Meaning |
|---|---|
| **D** | Documented: LG's site, or a project's own documentation |
| **S** | Read from source, at the commit listed under Sources |
| **R** | Reported by a third party on the hardware they name (issue thread, forum); not seen by this project |
| **I** | Inferred; the cell says from what |

## 1. Transport

| Fact | Tag | Source |
|---|---|---|
| SSAP is JSON in WebSocket text frames at `ws://<ip>:3000` and `wss://<ip>:3001`, with no path and no subprotocol. | S | [aiowebostv] `webos_client.py:41-42,182-183`; [lgtv2] `index.js:33-34,212-224` |
| LG's own SDK only connects with `wss`, to port 3001 or to the last number of the UPnP `serviceId` (`urn:lge-com:serviceId:webos-second-screen-3000-3001`). | S | [connect-sdk] `webos/WebOSTVServiceSocketClient.java:87,134-170`, `WebOSTVService.java:215-219` |
| 2022 models stopped accepting `ws` on 3000 with firmware 03.30.16 (webOS release 7.3.0; OLED42C2PUA, OLED77C29LD, OLED55G29LA), early 2023. The same TVs answer on 3001. | R | [openhab-14309] |
| bscpylgtv: "Use SSL for WebSocket connection by default (required by 2022 models)", v0.4.3. | D | [bscpylgtv] `CHANGELOG.md:119` |
| LG's developer forum, 2023-06: "the ws service on port 3000 is no longer functional"; LG's answer names Connect SDK and nothing else. | R | [lg-forum-3082] |
| On webOS 26 (OLED65G56LS, firmware 43.21.71) port 3000 accepts TCP and resets the WebSocket handshake; 3001 upgrades. | R | [aiowebostv-718] |
| `wss` on 3001 is reported to work on webOS 3, 4.5 and 5.0, and 3001 was open on a 2016 43UH6100 (webOS 3.0). lgtv2's README says TVs before 2018 only offer 3000. The two disagree. | R, D | [openhab-14309] comment of 2023-02-03; [h4ck] `README.md:38-54`; [lgtv2] `README.md:28-30` |
| Both maintained clients try `wss` 3001 first, fall back to `ws` 3000 and remember what worked. They bound the attempt (2 s connect, 10 s handshake) because a TV after power on can accept TCP and never answer. | S | [aiowebostv] `webos_client.py:30,174-200`; [lgtv2] `index.js:320-335,606-636` |
| No public CA verifies the TV's certificate. Every client turns verification off: `ssl=False`, `CERT_NONE`, `rejectUnauthorized: false`, or a trust manager that takes any chain. | S | [aiowebostv] `webos_client.py:156-163`; [bscpylgtv] `bscpylgtv/webos_client.py:131-134`; [lgtv2] `index.js:336-338`; [connect-sdk] `webos/WebOSTVTrustManager.java:31-53` |
| lgtv2 describes the chain: leaf "LGE TV SSG" (serial 0x2001), issuer "LGE SSG Intermediate CA" (0x1007), root "LG webOS TV Root CA", valid 2018-03-12 to 2034-08-15, the same certificate and key on every TV its author saw (2018 to 2023 models), and it can pin two SHA-256 fingerprints. Other sources call the certificate self-signed. | S | [lgtv2] `index.js:36-49`, `README.md:102-105` |
| Connect SDK stores the certificate at registration and requires the same one afterwards. When the certificate is not self-signed it compares the public key with one compiled into the SDK and sends a `verification` message. No third-party client sends that message. | S | [connect-sdk] `webos/WebOSTVServiceSocketClient.java:89-94,349-375,506-589` |
| A connection that carries an `Origin` header is closed with code 1008, "invalid origin" (a web page connecting to `ws://<tv>:3000`). LGWebOSRemote strips the header on the main and the input socket. `ws` 8 sends `Origin` only when `options.origin` is set. | R, S | [lg-forum-3360]; [lgwebosremote] `LGTV/remote.py:54-57`, `LGTV/cursor.py:15`; `ws` 8.22.1 `lib/websocket.js:810-814` |
| The TV must allow phone control: "enable LG Connect Apps feature in Network settings of the TV". A 2025 G5 on webOS 26 lists paired clients under Settings > General > External Devices > TV Mobile Connection Management. | D, R | [ha-webostv]; [aiowebostv-718] |
| Clients ping the WebSocket (every 5 s or 10 s): a TV going to standby does not always close the socket. | S | [aiowebostv] `webos_client.py:34,160`; [lgtv2] `index.js:27-31,508-534` |

## 2. Registration

### 2.1 The handshake

```
C: {"id":"hello","type":"hello","payload":{}}
T: {"type":"hello","payload":{"protocolVersion":1,"deviceType":"tv","deviceOS":"webOS",
    "deviceOSVersion":"4.1.0","deviceOSReleaseVersion":"11.2.0","deviceUUID":"<uuid>",
    "pairingTypes":["PIN","PROMPT","COMBINED","LGSWITCH-PIN"]}}
C: {"id":"get_sys_info","type":"request","uri":"ssap://system/getSystemInfo","payload":{}}
T: {"type":"response","id":"get_sys_info","payload":{"returnValue":true,"features":{"dvr":true},
    "receiverType":"DVB","modelName":"OLED65C25LB","serialNumber":"<serial>","programMode":true}}
C: {"type":"register","id":"register_0","payload":{"forcePairing":false,"pairingType":"PROMPT",
    "client-key":"<stored key, or absent>","manifest":{...}}}
T: {"type":"response","id":"register_0","payload":{"pairingType":"PROMPT","returnValue":true}}
T: {"type":"registered","id":"register_0","payload":{"client-key":"<key>"}}
```

| Fact | Tag | Source |
|---|---|---|
| The frames above: what aiowebostv sends, and what an OLED65C25LB on webOS 26 (release 11.2.0) answered in a user's debug log. The `response` frame is the acknowledgement LG's iOS SDK tests expect. | S, R | [aiowebostv] `webos_client.py:208-286`, `handshake.py:84-88`; [ha-172703] log of 2026-06-01; [connect-sdk-ios] `WebOSTVServiceSocketClientTests.m:68,74` |
| `hello` is optional: bscpylgtv sends it only on request. Connect SDK always sends it, with a payload that describes the client (`sdkVersion`, `deviceModel`, `OSVersion`, `resolution`, `appId`, `appName`, `appRegion`). | S | [bscpylgtv] `bscpylgtv/webos_client.py:234-243`; [connect-sdk] `webos/WebOSTVServiceSocketClient.java:437-504` |
| `hello` and `ssap://system/getSystemInfo` are answered before registration, with no prompt on the TV. aiowebostv asks for system info before registering because newer webOS may refuse it afterwards. | S, R | [aiowebostv] `webos_client.py:222-240,306-310`; [aiowebostv-718] |
| With a `client-key` the TV still accepts, it answers `registered` at once and shows nothing. Clients keep the stored key when `registered` carries none. | S | [lgtv2] `index.js:690-709`; [aiowebostv] `webos_client.py:278-286` |
| Without a key the TV answers `response` with `pairingType: "PROMPT"`, shows a connection request to accept with the remote, then sends `registered` with a new `client-key` (32 hex characters in bscpylgtv's example). | S, D | [lgtv2] `index.js:674-717`, `README.md:33-35`; [bscpylgtv] `README.md:130` |
| How long the TV keeps the prompt is not documented. aiowebostv waits 10 s for the `registered` frame; lgtv2 waits without limit. | S | [aiowebostv] `webos_client.py:31,268`; [lgtv2] `index.js:803-806` |
| A refusal is an `error` frame. Strings in client sources: `403 cancelled` ("when the user declines on the TV") and `403 User denied access`. Which one a TV sends for a decline and which for a prompt that timed out is not established. | S | [lgtv2] `index.js:686`, `test/mock-tv.js:79`; [aiowebostv-tests] `conftest.py:181` |
| PIN pairing: register with `pairingType: "PIN"`, the TV shows a PIN and answers `response` with `pairingType: "PIN"`; the client sends `{"type":"request","id":"register_1","uri":"ssap://pairing/setPin","payload":{"pin":"<pin>"}}` and gets `registered`. One client requires eight digits. Known types: `PROMPT`, `PIN`, `COMBINED`, `LGSWITCH-PIN`. | S | [bscpylgtv] `bscpylgtv/webos_client.py:250-271`, `bscpylgtv/constants.py:8`; [connect-sdk] `webos/WebOSTVServiceSocketClient.java:641-697`; [lgtvremote-cli] `lgtvremote_cli.py:469-481` |
| Every library sends `forcePairing: false`. What `true` does is not documented. | S | [aiowebostv] `handshake.py:15`; [lgtv2] `pairing.json:2` |
| The key is stored per TV and sent on every connection. Connect SDK forgets it when `hello.deviceUUID` is not the TV it was issued by, and changing the permission list means pairing again. | S | [lgtv2] `index.js:368-379,442-453`; [connect-sdk] `webos/WebOSTVServiceSocketClient.java:409-431`, `WebOSTVService.java:2517-2530` |

### 2.2 The manifest the libraries send

aiowebostv, bscpylgtv and lgtv2 send the same `signed` block and the same
signature, copied from one another for years ([aiowebostv] `handshake.py:3-81`,
[bscpylgtv] `bscpylgtv/manifest.py:1-77`, [lgtv2] `pairing.json:1-100`). **S**

```json
{
  "forcePairing": false,
  "pairingType": "PROMPT",
  "manifest": {
    "manifestVersion": 1,
    "appVersion": "1.1",
    "permissions": [
      "LAUNCH", "LAUNCH_WEBAPP", "APP_TO_APP", "CLOSE", "TEST_OPEN", "TEST_PROTECTED",
      "CONTROL_AUDIO", "CONTROL_DISPLAY", "CONTROL_INPUT_JOYSTICK",
      "CONTROL_INPUT_MEDIA_RECORDING", "CONTROL_INPUT_MEDIA_PLAYBACK", "CONTROL_INPUT_TV",
      "CONTROL_POWER", "CONTROL_TV_SCREEN", "READ_APP_STATUS", "READ_CURRENT_CHANNEL",
      "READ_INPUT_DEVICE_LIST", "READ_NETWORK_STATE", "READ_RUNNING_APPS",
      "READ_TV_CHANNEL_LIST", "WRITE_NOTIFICATION_TOAST", "READ_POWER_STATE",
      "READ_COUNTRY_INFO", "CONTROL_INPUT_TEXT", "CONTROL_MOUSE_AND_KEYBOARD",
      "READ_INSTALLED_APPS", "READ_SETTINGS"
    ],
    "signed": {
      "appId": "com.lge.test",
      "created": "20140509",
      "vendorId": "com.lge",
      "serial": "2f930e2d2cfe083771f68e4fe7bb07",
      "localizedAppNames": {"": "LG Remote App", "ko-KR": "리모컨 앱", "zxx-XX": "ЛГ Rэмotэ AПП"},
      "localizedVendorNames": {"": "LG Electronics"},
      "permissions": [
        "TEST_SECURE", "CONTROL_INPUT_TEXT", "CONTROL_MOUSE_AND_KEYBOARD", "READ_INSTALLED_APPS",
        "READ_LGE_SDX", "READ_NOTIFICATIONS", "SEARCH", "WRITE_SETTINGS",
        "WRITE_NOTIFICATION_ALERT", "CONTROL_POWER", "READ_CURRENT_CHANNEL",
        "READ_RUNNING_APPS", "READ_UPDATE_INFO", "UPDATE_FROM_REMOTE_APP",
        "READ_LGE_TV_INPUT_EVENTS", "READ_TV_CURRENT_TIME"
      ]
    },
    "signatures": [{
      "signatureVersion": 1,
      "signature": "eyJhbGdvcml0aG0iOiJSU0EtU0hBMjU2Iiwia2V5SWQiOiJ0ZXN0LXNpZ25pbmctY2VydCIsInNpZ25hdHVyZVZlcnNpb24iOjF9.hrVRgjCwXVvE2OOSpDZ58hR+59aFNwYDyjQgKk3auukd7pcegmE2CzPCa0bJ0ZsRAcKkCTJrWo5iDzNhMBWRyaMOv5zWSrthlf7G128qvIlpMT0YNY+n/FaOHE73uLrS/g7swl3/qH/BGFG2Hu4RlL48eb3lLKqTt2xKHdCs6Cd4RMfJPYnzgvI4BNrFUKsjkcu+WD4OO2A27Pq1n50cMchmcaXadJhGrOqH5YmHdOCj5NSHzJYrsW0HPlpuAx/ECMeIZYDh6RMqaFM2DXzdKX9NmmyqzJ3o/0lkk/N97gfVRLW5hA29yeAwaCViZNCP8iC9aO0q9fQojoa7NQnAtw=="
    }]
  }
}
```

The first part of the signature is base64 for
`{"algorithm":"RSA-SHA256","keyId":"test-signing-cert","signatureVersion":1}`:
it is LG's test signing certificate (**I**, from decoding it). bscpylgtv adds
`"deviceName": "bscpylgtv"` and `READ_STORAGE_DEVICE_LIST`; lgtv2 lists more
plain permissions (recording, channels, Bluetooth, `CONTROL_WOL`).

### 2.3 Newer firmware refuses that signature

| Fact | Tag | Source |
|---|---|---|
| webOS 26 (`deviceOSReleaseVersion` 11.x; G5, C6, and C2 once updated) answers the signed manifest with `{"type":"error","id":"register_0","error":"403 Pairing rejected: blacklisted certificate detected","payload":{}}` within milliseconds and shows no prompt. Keys issued earlier stop working too. One webOS 25 TV (65UT73006LA, 10.3.2) answers the same. | R | [ha-172703]; [aiowebostv-718]; [aiowebostv-720] |
| The `signatures` block alone triggers it: the same manifest without it gets a prompt and a key, whatever the `appId`. | R | [ha-172703] comment of 2026-08-18 (OLED65G56LS, 11.2.0) |
| What the libraries do now: aiowebostv retries without `signed` and `signatures`, with both permission lists merged; lgtv2 retries on that error text; bscpylgtv ships a "compatibility manifest for webOS26". LG's Android SDK only ever sent `manifestVersion` and `permissions`. | S | [aiowebostv] `webos_client.py:134-145,251-262`; [lgtv2] `index.js:19-25,680-685`; [bscpylgtv] `docs/manifests/README.md:3`; [connect-sdk] `webos/WebOSTVServiceSocketClient.java:222-234` |
| With an unsigned key on that G5, everything Home Assistant uses answered, the pointer input socket included. `com.webos.service.update/getCurrentSWInformation` answered `401 insufficient permissions`, and on a C6 so did `settings/setSystemSettings`. Listing the signed permissions in the plain list does not grant them. | R | [ha-172703] comments of 2026-08-18 and 2026-08-19 |
| An unsigned manifest must list `CONTROL_INPUT_TEXT` and `CONTROL_MOUSE_AND_KEYBOARD`, or `getPointerInputSocket` answers `401 insufficient permissions`. | D, R | [lgtv2] `CHANGELOG.md:19-25`; [lgtvcompanion-351] |

### 2.4 What a permission unlocks

LG publishes no mapping. Connect SDK groups them as open (`LAUNCH`,
`LAUNCH_WEBAPP`, `APP_TO_APP`, `CONTROL_AUDIO`, `CONTROL_INPUT_MEDIA_PLAYBACK`),
protected (`CONTROL_POWER`, `READ_INSTALLED_APPS`, `CONTROL_DISPLAY`,
`CONTROL_INPUT_JOYSTICK`, `CONTROL_INPUT_TV`, `READ_INPUT_DEVICE_LIST`,
`READ_NETWORK_STATE`, `READ_TV_CHANNEL_LIST`, `WRITE_NOTIFICATION_TOAST`, ...)
and personal activity (`CONTROL_INPUT_TEXT`, `CONTROL_MOUSE_AND_KEYBOARD`,
`READ_CURRENT_CHANNEL`, `READ_RUNNING_APPS`), and only offers keys, pointer,
text, toasts and power off once paired ([connect-sdk]
`webos/WebOSTVDeviceService.java:134-167`, `WebOSTVService.java:2415-2446`). **S**

| Permission | Unlocks | Tag |
|---|---|---|
| `CONTROL_MOUSE_AND_KEYBOARD`, `CONTROL_INPUT_TEXT` | `com.webos.service.networkinput/getPointerInputSocket` (section 4); `com.webos.service.ime/*` by its name | R (2.3); I |
| `READ_UPDATE_INFO` (signed only) | `com.webos.service.update/getCurrentSWInformation` | R (2.3) |
| `WRITE_SETTINGS` (signed only) | `settings/setSystemSettings` | R (2.3) |
| `LAUNCH`, `CLOSE`, `LAUNCH_WEBAPP` | `system.launcher/launch`, `open`, `close`; `webapp/*` | I (names, Connect SDK's groups) |
| `READ_INSTALLED_APPS`, `READ_RUNNING_APPS`, `READ_APP_STATUS` | `listApps`, `listLaunchPoints`, `getForegroundAppInfo`, `getAppStatus`. `listApps` answered with an unsigned key, though the signed list also names `READ_INSTALLED_APPS`. | I; R |
| `WRITE_NOTIFICATION_TOAST`; `WRITE_NOTIFICATION_ALERT` (signed only) | `system.notifications/createToast`; `createAlert` | I (names) |
| `CONTROL_POWER`, `READ_POWER_STATE`, `CONTROL_TV_SCREEN` | `system/turnOff`, `tvpower/power/getPowerState`, screen on and off | I (names) |
| `READ_NETWORK_STATE` | `com.webos.service.connectionmanager/getinfo` | I (name) |
| `TEST_OPEN`, `TEST_PROTECTED`, `TEST_SECURE` | the gateway's own test calls, such as `com.webos.service.secondscreen.gateway/test/secure` | I ([connect-sdk] `WebOSTVService.java:2299`) |
| `CONTROL_AUDIO`, `CONTROL_INPUT_MEDIA_PLAYBACK`, the channel and input ones | volume, `media.controls/*`, the tuner and HDMI inputs: nothing this CLI needs | I (names) |

## 3. Requests, subscriptions, errors

| Fact | Tag | Source |
|---|---|---|
| A request is `{"id": <id>, "type": "request", "uri": "ssap://<service>/<method>", "payload": {...}}`. The answer is `{"type": "response", "id": <same id>, "payload": {...}}`. Ids are the client's: integers, or strings such as `register_0`. | S | [aiowebostv] `webos_client.py:655-682`; [lgtv2] `index.js:754-813` |
| Success is `payload.returnValue: true`. A payload with `returnValue: false` carries `errorCode` and `errorText`. | S | [aiowebostv] `webos_client.py:713-738`; [lgtv2] `index.js:230-247` |
| A subscription is the same frame with `type: "subscribe"`; the TV then sends a `response` with that id on every change, the first usually with `subscribed: true` (webOS 6.0 omits it). `{"id": <id>, "type": "unsubscribe"}` stops it; the TV ignores the `uri` there. | S | [aiowebostv] `webos_client.py:768-797`; [lgtv2] `index.js:653-657,742-752` |
| An error is `{"type":"error","id":<id>,"error":"<code> <text>","payload":{}}`. Seen: `404 no such service or method`, `401 insufficient permissions`, `401 insufficient permissions (not registered)`, `403 access denied`, `500 Application error`. Connect SDK splits the string at the first space into a number and a text. | S, R | [aiowebostv] `webos_client.py:726-730`; [connect-sdk] `webos/WebOSTVServiceSocketClient.java:376-408`; [ha-172703]; [lgtv2] `README.md:252,262` |
| Services differ by webOS version: `com.webos.service.connectionmanager/getinfo` is a 404 on webOS 2.0 (65UF7700), and `com.webos.service.tvpower/*` is a 404 on webOS 4.4.3, where the service is `com.webos.service.tv.power`. A client must treat 404 as "not on this TV". | R | [ha-178411]; [ha-183602] |

## 4. Remote keys: the pointer input socket

| Fact | Tag | Source |
|---|---|---|
| `ssap://com.webos.service.networkinput/getPointerInputSocket` (no payload) answers `{"socketPath": "<ws or wss URL>", "returnValue": true}`. The client opens a second WebSocket to that URL, with the same TLS exception and no `Origin`. One design note shows `wss://<ip>:3001/resources/<token>/netinput.pointer.sock`. | S | [aiowebostv] `webos_client.py:288-297`; [lgtv2] `index.js:815-863`; [connect-sdk] `WebOSTVService.java:1487-1514`, `webos/WebOSTVMouseSocketConnection.java:70-117`; [asyncwebostv] `docs/design/pointer-protocol.md` |
| Frames on that socket are text, not JSON: `key:value` lines ended by an empty line. The TV sends nothing back and ignores fields it does not know. | S | [lgtv2] `index.js:289-304`; [aiowebostv] `webos_client.py:468-476`; [asyncwebostv] |
| A remote key: `type:button\nname:LEFT\n\n` | S | [connect-sdk] `webos/WebOSTVMouseSocketConnection.java:171-175`; [aiowebostv] `webos_client.py:816-819` |
| Pointer move: `type:move\ndx:<n>\ndy:<n>\ndown:0\n\n`, relative; `down:1` drags. | S | [connect-sdk] `webos/WebOSTVMouseSocketConnection.java:177-187` |
| Pointer click at the cursor: `type:click\n\n`. Scroll: `type:scroll\ndx:<n>\ndy:<n>\n\n`. | S | [connect-sdk] `webos/WebOSTVMouseSocketConnection.java:135-139,189-193` |
| Connect SDK's "OK" sends `type:click`, a pointer click, not the `ENTER` button. | S | [connect-sdk] `WebOSTVService.java:1690-1701` |
| LG's SDK itself sends only `HOME`, `BACK`, `UP`, `DOWN`, `LEFT`, `RIGHT`, `ENTER`, `DASH` and the digits. | S | [connect-sdk] `webos/WebOSTVMouseSocketConnection.java:47-54`, `WebOSTVService.java:1598-1622` |
| LGWebOSRemote waits 100 ms between buttons. | S | [lgwebosremote] `LGTV/cursor.py:51-52` |

Button names the clients know. No list comes from LG, and the clients say other
names may work ([ha-button] says "Other buttons supported by your TV may also
work"). **S**, [aiowebostv] `buttons.py:3-61`, [bscpylgtv] `bscpylgtv/buttons.py:1-79`,
[homebridge-webos-tv] `lib/LgTvController.js:67`:

- Navigation: `LEFT` `RIGHT` `UP` `DOWN` `ENTER` `BACK` `EXIT` `HOME` `MENU`
  `QMENU` `INFO` `GUIDE` `LIST` `RECENT` `MYAPPS` `INPUT_HUB` `SEARCH`
- Colours and digits: `RED` `GREEN` `YELLOW` `BLUE` `0` to `9` `DASH` `ASTERISK`
- Playback: `PLAY` `PAUSE` `STOP` `REWIND` `FASTFORWARD` `RECORD`. No client
  knows a play/pause toggle.
- Volume and channels: `VOLUMEUP` `VOLUMEDOWN` `MUTE` `CHANNELUP` `CHANNELDOWN`
- Other: `POWER` `CC` `AD` `SAP` `TELETEXT` `TEXTOPTION` `PROGRAM` `NETFLIX`
  `AMAZON` `3D_MODE` `ASPECT_RATIO` `LIVE_ZOOM` `MAGNIFIER_ZOOM` `SCREEN_REMOTE`
- Only in bscpylgtv: `ADVANCE_SETTING` `ALEXA` `EMANUAL` `EZPIC` `EZ_ADJUST`
  `EYE_Q` `HCEC` `IN_START` `IVI` `RECLIST` `SOCCER` `TIMER` `TV` `TWIN` `UPDOWN`
  `USP` `YANDEX`. Only in homebridge-webos-tv: `FAVORITES` `FLASHBACK`
  `GOTOPREV` `GOTONEXT` `EJECT`, and Japanese broadcast keys (`BS`, `CS1`, `CS2`,
  `TER` and their `_NUM_1` to `_NUM_12`).

**Holding a key is not possible over SSAP, as far as any source read here
shows.** `type:button` carries a name and nothing else; LG's SDK has
`button(String keyName)` and no press or release; no client has a hold, and a
search of client sources and forums found none. The TV's own Luna call for the
same service, `luna://com.webos.service.networkinput/sendSpecialKey {"key": ...}`
(used from a service on the TV, 65UR91006LB, webOS 10), is also one press
([cleanyourtv] `remotekeys.js:7-15`, **S**). Whether an app sees key down and key
up for one `type:button`, and whether fast repeated frames look like auto-repeat,
is not known.

## 5. Other calls

| Call | Payload and answer | Tag | Source |
|---|---|---|---|
| `ssap://system.launcher/launch` | `{"id": "<appId>", "params": {...}, "contentId": "..."}`; `params` and `contentId` optional. Answers `sessionId`. | S | [connect-sdk] `WebOSTVService.java:444-496`; [aiowebostv] `webos_client.py:898-912` |
| `ssap://com.webos.applicationManager/launch` | `{"id": "...", "params": {...}}`, the application manager called directly. | D | [lgtv2] `README.md:249` |
| `ssap://system.launcher/close` | `{"id": "<appId>"}`, and `sessionId` in Connect SDK. | S | [aiowebostv] `webos_client.py:914-916`; [connect-sdk] `WebOSTVService.java:622-639` |
| `ssap://com.webos.applicationManager/getForegroundAppInfo` | none; answers `appId`. Subscribable. | S | [aiowebostv] `webos_client.py:885-896` |
| `ssap://com.webos.applicationManager/listApps`, `listLaunchPoints` | none; `apps[]` (`id`, `title`, hidden apps included) and `launchPoints[]` (`id`, `title`, `icon`). | S | [connect-sdk] `WebOSTVService.java:641-667`; [aiowebostv] `webos_client.py:871-883` |
| `ssap://system/getSystemInfo` | none; `modelName`, `serialNumber`, `receiverType`, `features`, `programMode`. Answered before registration. | S, R | [aiowebostv] `webos_client.py:222-240`; [ha-172703] |
| `ssap://com.webos.service.update/getCurrentSWInformation` | none; `product_name` (`webOSTV 6.0`), `model_name` (`HE_DTV_W<yy><chip>_...`), `major_ver`, `minor_ver`. Signed manifests only on webOS 26 (2.3). More in [tv-discovery.md](tv-discovery.md), section 3. | S, R | [aiowebostv-tests] `conftest.py:26`; [homebridge-webos-tv] `index.js:342-344`; [bscpylgtv] `bscpylgtv/webos_client.py:1489-1501`; [ha-172703] |
| `ssap://com.webos.service.connectionmanager/getinfo` | none; `wiredInfo.macAddress`, `wifiInfo.macAddress`. | S | [lgtv2] `index.js:416-440` |
| `ssap://system/turnOff` | none. Clients do not wait for the answer: it may never come. | S | [aiowebostv] `webos_client.py:938-948` |
| `ssap://com.webos.service.ime/insertText` | `{"text": "abc", "replace": 0}`, into the text field that has the on-screen keyboard. | S | [connect-sdk] `webos/WebOSTVKeyboardInput.java:110-126` |
| `ssap://com.webos.service.ime/sendEnterKey`, `deleteCharacters` | none; `{"count": <n>}` | S | [connect-sdk] `webos/WebOSTVKeyboardInput.java:91-109` |
| `ssap://com.webos.service.ime/registerRemoteKeyboard` | subscription; `currentWidget.focus` says whether a text field is open. | S | [connect-sdk] `webos/WebOSTVKeyboardInput.java:44,149-222` |
| `ssap://system.notifications/createToast` | `{"message": "...", "iconData": "<base64>", "iconExtension": "png"}`; the icon is optional. | S | [aiowebostv] `webos_client.py:845-860` |
| `ssap://system.notifications/createAlert` | bscpylgtv uses its `onclose` and `onfail` hooks to call any `luna://` URI; the answer of that call is lost, and newer firmware closed several of those calls. | S | [bscpylgtv] `bscpylgtv/webos_client.py:1300-1327`, `bscpylgtv/endpoints.py:72-77` |

### Screenshot: `ssap://tv/executeOneShot`

| Fact | Tag | Source |
|---|---|---|
| The call exists in bscpylgtv since v0.4.4 (2023) with no payload: "Take screenshot in 960x540px JPG format". Others send `{"path": "/tmp/capture.jpg", "method": "DISPLAY", "format": "JPG", "width": 960, "height": 540}`. | S | [bscpylgtv] `bscpylgtv/endpoints.py:41`, `bscpylgtv/webos_client.py:1038-1045`, `CHANGELOG.md:113`; [glowcast] `custom_components/glowcast/sources/webos.py:95-104` |
| The answer has `imageUri`. On a G5 with webOS 25 it is an HTTPS URL; one project's fixture has the shape `https://<tv>:3001/resources/<token>/capture.jpg`. Older notes say "HTTP link". The download needs the same TLS exception as the socket. | R, S | [clean-webos-tv-2]; [glowcast] `tests/test_webos_source.py:34`, `custom_components/glowcast/sources/webos.py:271-276`; [webos-cheatsheet] |
| The image is 960x540 JPEG whatever is asked: "hard-locked to 960x540 JPEG on every webOS version tested (B9, CX, G5)"; on a CX `format` PNG, RGB and YUV422 still gave the JPEG, and parallel calls gave one frame. About 2.5 to 4 captures a second. | D, R | [lgtvremote-cli] README; [lg-webos-tv-3]; [glowcast-26] |
| It works with the ordinary pairing and no extra prompt on a 2023 G3. One owner of a 43UH603V reports the TV asking for confirmation before the screenshot. Which permission gates the call is not known; a TV can answer 404 or 401. | R, S | [glowcast-2]; [webos-forums-ru] post of 2025-03-18; [glowcast] `custom_components/glowcast/sources/webos.py:197-206` |
| `DISPLAY` is the composited picture. Called on the TV's capture service directly (49SM9000PLA, webOS 4.10, rooted): "contains both the video plane and the webOS UI". Through SSAP, a streaming app gives its UI around a black video area, and Prime Video on a G3 gave frames with every pixel 0. | R, S | [dev-manager-419]; [kiwaili] `capture.py:1-8,102-106`; [glowcast-14] |
| The Luna call `com.webos.service.apiadapter/tv/executeOneShot` refuses a Developer Mode shell (`luna-send-pub`, CX). By its name it is what SSAP forwards to. | R, I (the name) | [lg-webos-tv-3] |
| The services behind it, `luna://com.webos.service.capture/executeOneShot` (older TVs: `com.webos.service.tv.capture`), take `path`, `method` (`DISPLAY`, `VIDEO`, `GRAPHIC`, ...), `format` (`PNG`, `JPG`, `BMP`, ...), `width`, `height`; one user captures 3840x2160. Dev Manager only offers them when it is logged in as `root`. webOS 4 needs a size (`CAPTURE_ERROR_03`). | D, S, R | [webosbrew-cheatsheet]; [webos-forums-ru] post of 2024-05-27; [dev-manager] `core/services/device-manager.service.ts:148-182`, `info/info.component.ts:105-108`; [dev-manager-419] |
| `ares-device --capture-screen` is refused for the `tv` profile; it is for webOS OSE. LG's CLI guide does not list it. | S, D | `@webos-tools/cli` 3.2.5 `bin/ares-device.js:193-197`; [lg-cli] |

So without root there are two ways to get a picture: this call (960x540 JPEG,
undocumented, pairing needed), or the app reading its own framebuffer with
`glReadPixels` and sending it to the CLI, which needs nothing from the TV.

## 6. Wake-on-LAN

A magic packet is 6 bytes `FF` and the MAC 16 times, 102 bytes, sent by UDP
broadcast (lgtv2: port 9, three packets 100 ms apart, to `255.255.255.255` or
the subnet's broadcast address) ([lgtv2] `index.js:81-171`, **S**). The TV must
allow it: Settings > General > Mobile TV On > Turn on via Wi-Fi from 2017, and
Settings > Support > IP control settings > Wake on LAN on 2025 models
([ha-webostv], **D**). The TV listens on the interface it uses, so clients keep
both MACs from `connectionmanager/getinfo` and send to both ([lgtv2]
`README.md:170-177`, **D**). SSAP cannot turn a TV on: an off TV has closed its
socket. A webOS 6.0 OLED closed the socket about 4 s after `turnOff`, port 3001
about 10 s later, and answered again about 2 s after the packet ([lgtv2]
`README.md:160-168`, **R**).

## Open questions

- How long the pairing prompt stays, and the exact error for a decline and for a
  timeout.
- What the prompt names the client when the manifest is unsigned, and whether a
  field (`deviceName`, `appId`, `localizedAppNames` outside `signed`) sets it.
- What one `type:button` is to an app: key down and up, one after the other?
  Do repeated frames reach it as a held key?
- Whether `tv/executeOneShot` answers on webOS 26, with an unsigned key, and
  which permission it needs. Whether a native app's own surface is in the
  `DISPLAY` capture: inferred yes from the UI of streaming apps, not seen.
- Whether `system.launcher/launch` starts an app installed in Developer Mode and
  hands `params` to a native app the way `ares-launch --params` does.
- The oldest webOS where 3001 answers, and whether Node 22's TLS defaults (OpenSSL
  3) complete a handshake with older TVs.
- Whether the TV wants Connect SDK's `verification` message on any firmware.
- Which manifest fields the TV checks: Home Assistant's fix drops `signed` and
  `signatures`, lgtv2's drops `signed` and keeps `signatures`, and both are said
  to pair on webOS 26.
- Whether Bun's WebSocket sends an `Origin` header (`ws` under Node does not).
- LG's other protocol, "IP control" on TCP 9761 (AES, a keycode shown in a
  hidden menu, `KEY_ACTION` commands, per [snapone-ip-control]): whether it can
  hold a key. Not read in full.

## What this means for react-native-webos

- `rn-webos key` connects to `wss://<ip>:3001` first, then `ws://<ip>:3000`,
  with certificate verification off for that socket only and no `Origin`. The
  second socket and the screenshot download get the same treatment.
- It registers with an unsigned manifest (`manifestVersion`, `appVersion`,
  `permissions` including `CONTROL_INPUT_TEXT`, `CONTROL_MOUSE_AND_KEYBOARD`,
  `LAUNCH`, `CLOSE`, `READ_INSTALLED_APPS`, `READ_RUNNING_APPS`,
  `WRITE_NOTIFICATION_TOAST`, `CONTROL_POWER`, `READ_NETWORK_STATE`). The
  `com.lge.test` signature is somebody else's identity, is refused by webOS 26,
  and buys nothing this CLI needs.
- It dispatches on the frame `type`, not on its position: `registered`,
  `response` with `pairingType`, or `error` can each be the first answer. It
  prints the TV's own error text.
- The client key is stored like the Samsung token (mode 0600), keyed by
  `hello.deviceUUID` rather than by address, and replaced whenever `registered`
  carries another one.
- `hello` and `getSystemInfo` are sent before `register`: they identify the TV
  without a prompt ([tv-discovery.md](tv-discovery.md)).
- Keys are `type:button` frames with the names of section 4, a pause between
  them, and `ENTER` for select (never `type:click`, which is a pointer click).
- A long press cannot be sent to the TV. Tests that hold a key need the app
  itself: the host injects key down and key up into its own input pipeline when
  the CLI asks over the app's development channel. The simulator keeps doing it
  natively.
- `rn-webos screenshot` asks the app for its framebuffer. `tv/executeOneShot` is
  a fallback for a picture of the whole TV, at 960x540.
- Every 404 and 401 is a normal answer: services move between webOS versions.

## Sources

[aiowebostv]: https://github.com/home-assistant-libs/aiowebostv/tree/f24d86bac901f67b61d6cf397681505f27b0c025/aiowebostv
[aiowebostv-tests]: https://github.com/home-assistant-libs/aiowebostv/tree/f24d86bac901f67b61d6cf397681505f27b0c025/tests
[bscpylgtv]: https://github.com/chros73/bscpylgtv/tree/191e8957a164a7a15b0b687071b534394a161aed
[lgtv2]: https://github.com/hobbyquaker/lgtv2/tree/80ef796846730cb4331f33be38221de7485afc55
[connect-sdk]: https://github.com/ConnectSDK/Connect-SDK-Android-Core/tree/2f7ebbc7eb909321ff41b2c6bcdf6b0881427579/src/com/connectsdk/service
[connect-sdk-ios]: https://github.com/ConnectSDK/Connect-SDK-iOS-Core/blob/8ea9070b71779c8e6714fa820a9fdb140f89ce03/ConnectSDKTests/Services/Helpers/WebOSTVServiceSocketClientTests.m
[lgwebosremote]: https://github.com/klattimer/LGWebOSRemote/tree/d2923a94949a5af30c058b7dd9e2464b11ce67a0
[homebridge-webos-tv]: https://github.com/merdok/homebridge-webos-tv/tree/d5590099f8a6e8276d348a7bd4953259a5135425
[lgtvremote-cli]: https://github.com/griches/lgtvremote-cli/tree/5770baf506637e635d9381439cbcb48b7257c261
[asyncwebostv]: https://github.com/locveil/asyncwebostv/blob/c21c3b2445ab57bbe54db372c9664c2ffa5e1d2d/docs/design/pointer-protocol.md
[cleanyourtv]: https://github.com/lasharor/cleanyourtv-WebOS/blob/6aee5f5d9e4c43babcdf9ee7189dcf6780db6e4f/addons/catalog/phoneremote/remotekeys.js
[glowcast]: https://github.com/nicohabets/glowcast/tree/21e11338c73f994055b7d2c0921cd303bc7d1e97
[glowcast-2]: https://github.com/nicohabets/glowcast/issues/2
[glowcast-14]: https://github.com/nicohabets/glowcast/issues/14
[glowcast-26]: https://github.com/nicohabets/glowcast/pull/26
[kiwaili]: https://github.com/kiwaili/home-assistant-lg-tv/blob/0c5fb6b5744cdeffc09905a2de8d8df1541d5af2/custom_components/lg_tv_screen/capture.py
[lg-webos-tv-3]: https://github.com/ishiishikou/lg-webos-tv/issues/3
[clean-webos-tv-2]: https://github.com/kaaburgh/clean-webos-tv/pull/2
[dev-manager]: https://github.com/webosbrew/dev-manager-desktop/tree/0251413b90eddbba64cee0b59f09070567d2a501/src/app
[dev-manager-419]: https://github.com/webosbrew/dev-manager-desktop/issues/419
[webosbrew-cheatsheet]: https://www.webosbrew.org/pages/commands-cheatsheet.html
[webos-cheatsheet]: https://gist.github.com/AkdM/c1571b2b0577f1bdb16011eb9b905803
[webos-forums-ru]: https://webos-forums.ru/post151765.html
[h4ck]: https://github.com/chorankates/h4ck/blob/7f2e0aee15d9985fd9819af41362749a5ebbe94d/lg_webOS/README.md
[ha-172703]: https://github.com/home-assistant/core/issues/172703
[ha-178411]: https://github.com/home-assistant/core/issues/178411
[ha-183602]: https://github.com/home-assistant/core/issues/183602
[aiowebostv-718]: https://github.com/home-assistant-libs/aiowebostv/issues/718
[aiowebostv-720]: https://github.com/home-assistant-libs/aiowebostv/issues/720
[openhab-14309]: https://github.com/openhab/openhab-addons/issues/14309
[lgtvcompanion-351]: https://github.com/JPersson77/LGTVCompanion/issues/351
[lg-forum-3082]: https://forum.webostv.developer.lge.com/t/connecting-mobile-apps-to-lg-webos-tv/3082
[lg-forum-3360]: https://forum.webostv.developer.lge.com/t/socket-connection-to-lg-tv/3360
[lg-cli]: https://webostv.developer.lge.com/develop/tools/cli-dev-guide
[ha-webostv]: https://www.home-assistant.io/integrations/webostv/
[ha-button]: https://www.home-assistant.io/actions/webostv.button/
[snapone-ip-control]: https://help.snapone.com/sb-v4-ig/Content/Topics/IP%20Control%20Guide.htm
