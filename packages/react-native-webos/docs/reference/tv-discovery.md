# Finding LG webOS TVs on the network

How `rn-webos devices` can find an LG TV from the computer, learn its name,
model, webOS version and a stable id, and tell whether Developer Mode is on:
SSDP, the TV's UPnP description, the first two messages of its remote-control
socket, and its Developer Mode ports. The remote-control protocol itself is in
[webos-remote-control.md](webos-remote-control.md). Researched on 2026-11-11
from documentation and source only: no TV was contacted, and nothing below is
verified on hardware by this project.

| Tag | Meaning |
|---|---|
| **D** | Documented: LG's site, or a project's own documentation |
| **S** | Read from source, at the commit listed under Sources |
| **R** | Reported by a third party on the hardware they name (issue thread, forum, a capture kept as a test fixture); not seen by this project |
| **I** | Inferred; the cell says from what |

## 1. SSDP

The search every client sends, to UDP `239.255.255.250:1900`, with `MX` between
2 and 5 ([connect-sdk] `discovery/provider/ssdp/SSDPClient.java:37-38,144-158`,
[lgwebosremote] `LGTV/scan.py:8-12`, **S**):

```
M-SEARCH * HTTP/1.1
HOST: 239.255.255.250:1900
MAN: "ssdp:discover"
MX: 3
ST: urn:lge-com:service:webos-second-screen:1

```

### 1.1 What a TV answers

| Fact | Tag | Source |
|---|---|---|
| A TV is three UPnP root devices, each with its own UUID and its own HTTP port. On an OLED55B7V (webOS 3.8): `:1201` Basic device with the second-screen service, `:1328` MediaRenderer, `:1412` DIAL. | R | [openhab-60640] |
| `ST: urn:lge-com:service:webos-second-screen:1` is answered by the Basic device (`urn:schemas-upnp-org:device:Basic:1`). It is the target LG's Connect SDK and Home Assistant search. | S, R | [connect-sdk] `service/WebOSTVService.java:228-230`; [ha-core] `homeassistant/components/webostv/manifest.json:14-18`; [openhab-60640] |
| `ST: urn:schemas-upnp-org:device:MediaRenderer:1` is answered by the DLNA renderer (AVTransport, ConnectionManager, RenderingControl). It is the target LG's `ares` CLI and LGWebOSRemote search. | S, R | `ssdp-js` 1.0.1 `index.js:70`; [lgwebosremote] `LGTV/scan.py:12`; [openhab-60640] |
| `ST: urn:dial-multiscreen-org:service:dial:1` and `upnp:rootdevice` are answered too. | R | [openhab-60640] |
| LG's IP control guide, as a control vendor republishes it, names the same three: Basic, DIAL, MediaRenderer; "from the Basic device", manufacturer, address and model number; `ssdp:alive` at power on and `ssdp:byebye` at power off. | D | [snapone-ip-control] |
| `urn:lge:device:tv:1` appears in no source read. | | |
| The ports differ per TV and per device (1201, 1328, 1412 above; 1702 in the capture below; 1175 on a 43UH6100). One client that polled a fixed port, 1426, calls its discovery "completely broken". Only `LOCATION` says where a description is. | R, D | [openhab-60640]; [nitpick]; [h4ck] `README.md:38`; [go-lgtv] |
| The TV must be on: "The TV must be turned on to be detected by SSDP." | D | [gladys] |
| Clients send the search more than once: three rounds to the group and to `255.255.255.255`, four rounds 2 s apart, or again every 5 s. | S | [lgtvremote-cli] `lgtvremote_cli.py:578-612`; [lgwebosremote] `LGTV/scan.py:18-20,48`; `ssdp-js` 1.0.1 `ssdp.js:14-15,80-84` |

A response to the second-screen search, which one client keeps as its "captured
SSDP response" test ([nitpick] `DiscoveryTests.swift:8-13`, **R**; the address
and the UUID are replaced here):

```
HTTP/1.1 200 OK
LOCATION: http://<ip>:1702/
CACHE-CONTROL: max-age=1800
EXT:
USN: uuid:<uuid>::urn:lge-com:service:webos-second-screen:1
ST: urn:lge-com:service:webos-second-screen:1
DATE: Sun, 17 Jan 2016 13:52:40 GMT
SERVER: WebOS/4.1.0 UPnP/1.0
```

| Fact | Tag | Source |
|---|---|---|
| `USN` is `uuid:<uuid>::<search target>`; clients cut the UUID out of it. | S | [connect-sdk] `discovery/provider/SSDPDiscoveryProvider.java:81,310-320`; `@webos-tools/cli` 3.2.5 `bin/ares-setup-device.js:470` |
| `SERVER: WebOS/4.1.0 UPnP/1.0` is not the webOS release. `4.1.0` is also `hello.deviceOSVersion` on TVs whose release is 4.4.2, 7.3.0 and 11.2.0. Connect SDK reads the first token as the service's version and special-cases `4.0.0` and `4.0.1`. | I (from the three reports), S | [openhab-14309]; [ha-172703]; [connect-sdk] `service/WebOSTVService.java:200-207,2449-2451` |
| The MediaRenderer's answer adds `DLNADeviceName.lge.com: <name>`, the friendly name URL-encoded (`%5BLG%5D%20webOS%20TV%20<model>` by default). Two clients decode it instead of fetching the description. | S | [lgwebosremote] `LGTV/scan.py:33-36`; [smartthings-lgtv] `hubpackage/src/discovery.lua:90-100` |
| A 2015 test double of an LG description response has the HTTP headers `Server: Linux/i686 UPnP/1,0 DLNADOC/1.50 LGE WebOS TV/Version 0.9` and `DLNADeviceName.lge.com: LG%20Smart%2b%20TV`. `ares` keeps the MediaRenderer answers whose `SERVER` contains `WebOS`. | S | [connect-sdk-windows] `Fakes/MessageFakeFactory.cs:86-90`; `@webos-tools/cli` 3.2.5 `bin/ares-setup-device.js:457-461` |

### 1.2 The description at `LOCATION`

The second-screen device's description. The structure is LG's SDK test fixture
([connect-sdk-tests] `SSDPDeviceTest.java:22-57`, **S**, where the name is
`Adnan TV` and the model number `WEBOS1`); the name and model number are those
of a 2024 OLED in another client's fixture ([nitpick]
`DiscoveryTests.swift:23-37`, **R**).

```xml
<root xmlns="urn:schemas-upnp-org:device-1-0" xmlns:dlna="urn:schemas-dlna-org:device-1-0">
  <specVersion><major>1</major><minor>0</minor></specVersion>
  <device>
    <deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>
    <friendlyName>[LG] webOS TV OLED65B4PUA</friendlyName>
    <manufacturer>LG Electronics</manufacturer>
    <manufacturerURL>http://www.lge.com</manufacturerURL>
    <modelDescription/>
    <modelName>LG Smart TV</modelName>
    <modelURL>http://www.lge.com</modelURL>
    <modelNumber>OLED65B4PUA</modelNumber>
    <serialNumber/>
    <UDN>uuid:(the uuid of the USN)</UDN>
    <serviceList>
      <service>
        <serviceType>urn:lge-com:service:webos-second-screen:1</serviceType>
        <serviceId>urn:lge-com:serviceId:webos-second-screen-3000-3001</serviceId>
        <SCPDURL>/WebOS_SecondScreen/(uuid)/scpd.xml</SCPDURL>
        <controlURL>/WebOS_SecondScreen/(uuid)/control.xml</controlURL>
        <eventSubURL>/WebOS_SecondScreen/(uuid)/event.xml</eventSubURL>
      </service>
    </serviceList>
  </device>
</root>
```

| Fact | Tag | Source |
|---|---|---|
| `modelName` is the constant `LG Smart TV`; the model is in `modelNumber`. An old Home Assistant matcher is exactly `deviceType` Basic plus `modelName` `LG Smart TV`. | S | [netdisco]; [lgtvremote-cli] `lgtvremote_cli.py:675-690` |
| `friendlyName` defaults to `[LG] webOS TV <model>`; LG's fixture has a name its owner chose (`Adnan TV`). Home Assistant shows it with `[LG]` replaced by `LG`. | S | [ha-core] `homeassistant/components/webostv/config_flow.py:122-124`, `tests/components/webostv/test_config_flow.py:43` |
| `serviceId` ends with the two socket ports; Connect SDK takes the last number as the port to connect to. | S | [connect-sdk] `service/WebOSTVService.java:215-219` |
| LG's own extensions that Connect SDK reads: `<appCasting>support</appCasting>` and `<supportAppcastingFeatures>mirroring\|remote-camera</supportAppcastingFeatures>` (screen mirroring from a phone). | S | [connect-sdk] `service/WebOSTVService.java:2481-2492` |
| One client also looks for `<bluetoothMac>`, "some TVs include this". No other `X_` or LG field was found in a source. | S | [lgtvremote-cli] `lgtvremote_cli.py:692-700` |

## 2. mDNS and DNS-SD

No source read documents what an LG TV advertises over mDNS, or its TXT records.

| Fact | Tag | Source |
|---|---|---|
| Home Assistant finds LG TVs by SSDP only: its generated tables have one `webostv` entry, the second-screen search target, and no DNS-SD type for it. | S | [ha-core] `homeassistant/generated/ssdp.py:414-416`, `homeassistant/components/webostv/manifest.json:14-18` |
| LG's CLI, Connect SDK's webOS service, and every remote-control client read for this file use SSDP, never mDNS. | S | sections 1 and 4 |
| One phone app's pull request adds `_airplay._tcp` for "LG webOS 2018+" and `_googlecast._tcp`, and strips `[LG]` from the instance name. It does not say it was run against a TV. | D | [dvdremote-8] |
| `_lg-smart-device._tcp` and `_display._tcp` appear in no source read. | | |

On this evidence mDNS identifies nothing reliably: AirPlay and Cast types are
shared with every other brand, and nothing read says which TXT key would carry
an LG model. SSDP is what LG's own tools use.

## 3. What a TV says about itself

### 3.1 Without pairing

Besides the description, the remote-control socket answers two messages before
any registration, with no prompt on the TV ([aiowebostv]
`webos_client.py:208-240,406-410`, **S**; [aiowebostv-718]: "`hello` and
`ssap://system/getSystemInfo` both succeed. The failure occurs only at the
registration step", **R**):

```
C: {"id":"hello","type":"hello","payload":{}}
T: {"type":"hello","payload":{"protocolVersion":1,"deviceType":"tv","deviceOS":"webOS",
    "deviceOSVersion":"4.1.0","deviceOSReleaseVersion":"11.2.0","deviceUUID":"<uuid>",
    "pairingTypes":["PIN","PROMPT","COMBINED","LGSWITCH-PIN"]}}
C: {"id":"1","type":"request","uri":"ssap://system/getSystemInfo","payload":{}}
T: {"type":"response","id":"1","payload":{"returnValue":true,"features":{"dvr":true},
    "receiverType":"DVB","modelName":"OLED65G56LS.DEUQLJP","serialNumber":"<serial>","programMode":true}}
```

| Wanted | Field | Tag | Source |
|---|---|---|---|
| Name | description `friendlyName`; `DLNADeviceName.lge.com` | S | section 1 |
| Model | description `modelNumber` (`OLED65B4PUA`); `getSystemInfo.modelName` (`OLED65C25LB`, or with the region suffix `OLED65G56LS.DEUQLJP`) | R | [nitpick]; [ha-172703] logs of 2026-06-01 and 2026-08-18 |
| webOS version | `hello.deviceOSReleaseVersion`. Seen: `1.3.2` (LG's SDK fixture), `4.4.2`, `4.4.3`, `7.3.0`, `10.3.2`, `11.2.0`. | S, R | [connect-sdk-ios] `WebOSTVServiceSocketClientTests.m:68`; [openhab-14309]; [ha-183602]; [aiowebostv-720]; [ha-172703] |
| Stable id | `hello.deviceUUID`; description `UDN` | S | section 5 |
| Serial number | `getSystemInfo.serialNumber`, readable by anyone on the network | R | [ha-172703] |

### 3.2 With pairing

| Call | Fields | Tag | Source |
|---|---|---|---|
| `ssap://com.webos.service.update/getCurrentSWInformation` | `product_name` (`webOSTV 6.0`, `webOSTV 25`), `model_name` (the platform, `HE_DTV_W<yy><chip>_...`), `major_ver` and `minor_ver` (Home Assistant joins them with a dot as the firmware version), `device_id`. On webOS 26 it answers `401 insufficient permissions` to a client paired without LG's signed manifest. | S, D, R | [homebridge-webos-tv] `index.js:342-344`, `lib/LgTvController.js:1593-1605`; [ha-core] `homeassistant/components/webostv/media_player.py:185-188`; [aiowebostv-tests] `conftest.py:26`; [lgwebosremote] `README.md:66-67`; [ha-172703] |
| `ssap://com.webos.service.connectionmanager/getinfo` | `wiredInfo.macAddress`, `wifiInfo.macAddress`. A 404 on webOS 2.0 (65UF7700). | S, R | [lgtv2] `index.js:416-440`; [ha-178411] |

`device_id` is treated as private next to `deviceUUID` and `macAddress`
([ha-core] `homeassistant/components/webostv/diagnostics.py:13-22`, **S**); the
same-named field of a 43UH6100's firmware update request is a MAC address
([h4ck] `README.md:96-102`, **R**).

### 3.3 With Developer Mode SSH

webOS Dev Manager reads, with `luna-send-pub` as the Developer Mode user
([dev-manager] `src/app/core/services/device-manager.service.ts:110-132`,
`src/app/core/services/remote-luna.service.ts:27-32`, **S**):

- `luna://com.webos.service.tv.systemproperty/getSystemInfo` with
  `{"keys": ["firmwareVersion", "modelName", "sdkVersion", "otaId"]}`. It is the
  call behind `ares-device --system-info` (`@webos-tools/cli` 3.2.5
  `files/conf/command-service.json:67-71`).
- `luna://com.palm.systemservice/osInfo/query` with `{"parameters":
  ["device_name", "webos_manufacturing_version", "webos_release"]}`:
  `webos_release` is the webOS version, `device_name` the chipset.

### 3.4 Release numbers, firmware and model years

| TV (year) | Firmware | webOS release | Source |
|---|---|---|---|
| 43UH6100 (2016) | 4.30.40 | `webOSTV 3.0`, platform `HE_DTV_W16P_...` | [h4ck] `README.md:16-20,96` (R) |
| OLED55B7V (2017) | | 3.8.0 | [openhab-60640] (R) |
| OLED C8 (2018) | 5.50.70 | 4.4.3 | [bscpylgtv] `docs/available_settings_C8.md:1` (D) |
| OLED C9 (2019) | 5.50.00 | 4.10.2 | [bscpylgtv] `docs/available_settings_C9.md:1` (D) |
| OLED CX (2020) | 4.63.25 | 5.6.1 | [bscpylgtv] `docs/available_settings_CX.md:1` (D) |
| OLED C1 (2021) | 3.53.45 | 6.5.3 | [bscpylgtv] `docs/available_settings_C1.md:1` (D) |
| OLED C2, G2 (2022) | 03.30.16 | 7.3.0, "webOS22" | [openhab-14309] (R) |
| OLED G3 (2023) | 23.25.55 | 9.2.2 | [bscpylgtv] `docs/available_settings_G3.md:1` (D) |
| OLED G4 (2024) | 23.23.30 | 9.2.4 | [bscpylgtv] `docs/available_settings_G4.md:1` (D) |
| OLED G5 (2025) | 33.21.81 | 10.2.0, `webOSTV 25` for firmware 33 | [bscpylgtv] `docs/available_settings_G5.md:1` (D); [lgwebosremote] `README.md:67` (D) |
| OLED G6 (2026) | 43.11.77 | 11.1.0 | [bscpylgtv] `docs/available_settings_G6.md:1` (D) |
| OLED C2 (2022), updated | 43.21.74 | 11.2.0, webOS 26 | [bscpylgtv] `docs/available_settings_C2.md:1` (D); [ha-172703] (R) |

- From 2022 the release's major number counts the yearly versions: 7 is webOS
  22, 10 is webOS 25, 11 is webOS 26; 8 as webOS 23 and 9 as webOS 24 follow.
  **I**, from the rows above.
- A TV is upgraded across versions (the 2022 C2 above runs 11.2.0), so the
  release does not give the model year. **I**, same rows.
- The model year is in the platform name of `getCurrentSWInformation.model_name`:
  `HE_DTV_W<yy><chip>_...`, with `W18` to `W26` in bscpylgtv's table of chips and
  `W16P` on a 2016 TV. **I**, from [bscpylgtv] `bscpylgtv/webos_client.py:1489-1501`
  and [h4ck].
- For OLEDs the model number carries the series, whose digit follows the year
  from 2021: C1 2021, C2 2022, G3 2023, G4 2024, G5 2025, G6 2026. **I**, from
  bscpylgtv's file titles. LCD model numbers were not studied.

## 4. How other tools find TVs

| Tool | How | Tag | Source |
|---|---|---|---|
| LG's `ares-setup-device --search` | "Searches webOS devices in the same network with SSDP". It searches MediaRenderer, fetches each `LOCATION` for `friendlyName`, keeps answers whose `SERVER` contains `WebOS`, takes the UUID from `USN`, waits 5 s (`--timeout`), then offers to add the TV. The option is in LG's guide and not in the installed help text. | D, S | [lg-cli]; `@webos-tools/cli` 3.2.5 `bin/ares-setup-device.js:46,65,90-91,445-486`; `ssdp-js` 1.0.1 `index.js:25-36,51-70` |
| LG's Connect SDK | SSDP for the second-screen target; UUID from `USN`; `friendlyName`, `modelName`, `modelNumber`, `manufacturer`, `UDN` from the description. | S | [connect-sdk] `service/WebOSTVService.java:228-230`, `discovery/provider/SSDPDiscoveryProvider.java:310-403` |
| Home Assistant | SSDP for the second-screen target; host from `LOCATION`; name from `friendlyName`; unique id from `UDN` without `uuid:`. | S | [ha-core] `homeassistant/components/webostv/config_flow.py:113-135` |
| webOS Dev Manager | No discovery: the user types the address. It then tries TCP 22, 9922 and 9991, 10 s each. | S | [dev-manager] `src-tauri/src/device_manager/manager.rs:129-141` |
| lgtvremote-cli | Second-screen SSDP, then the description. As a fallback it tries HTTP on port 3000 (`POST /api/com.webos.service.tv.systemproperty/getSystemInfo`, "newer webOS (5.0+) only"). Unconfirmed anywhere else. | S | [lgtvremote-cli] `lgtvremote_cli.py:578-612,651-737` |

## 5. A stable identity

| Fact | Tag | Source |
|---|---|---|
| The second-screen device's `UDN` and `hello.deviceUUID` are used as one id: Home Assistant sets the same unique id from either, depending on how the TV was added. | I (from that code) | [ha-core] `homeassistant/components/webostv/config_flow.py:98-101,126-130` |
| LG's SDK treats `deviceUUID` as the TV's identity: when it differs from the stored one, the SDK drops the client key and disconnects. bscpylgtv offers `hello` "to get unique deviceUUID". | S, D | [connect-sdk] `service/webos/WebOSTVServiceSocketClient.java:409-431`; [bscpylgtv] `README.md:125` |
| The MediaRenderer and DIAL devices of the same TV have other UUIDs. What `ares-setup-device --search` records is the MediaRenderer's. | R, S | [openhab-60640]; `ssdp-js` 1.0.1 `index.js:70` |
| SSDP does not give a MAC address: "SSDP does not reliably expose the TV MAC address." Clients read it from the ARP table, or after pairing from `connectionmanager/getinfo`, and keep the wired and the Wi-Fi one for Wake-on-LAN. | D, S | [gladys]; [lgtv-menubar]; [lgtv2] `index.js:388-440` |
| Clients that key their stored client key by address (lgtv2 by host, bscpylgtv by IP) lose the pairing when DHCP moves the TV. | I (from that code) | [lgtv2] `index.js:368-371`; [bscpylgtv] `bscpylgtv/webos_client.py:158-164` |

## 6. Developer Mode ports

| Fact | Tag | Source |
|---|---|---|
| Developer Mode's SSH server listens on 9922, user `prisoner`. | D, S | [lg-devmode]; `@webos-tools/cli` 3.2.5 `files/conf/config.json:16-17` |
| openlgtv's notes start that server by hand from the Developer Mode service's own files, with `-p 9922`: it belongs to Developer Mode, not to the base system. | D | [openlgtv-notes] |
| The key server is `http://<ip>:9991/webos_rsa`. LG's steps have the user press "Key Server" in the Developer Mode app before `ares-novacom --getkey`. | S, D | `@webos-tools/cli` 3.2.5 `lib/base/novacom.js:370`; [lg-devmode] |
| Dev Manager tells what an address offers by connecting to 22, 9922 and 9991. Port 22 is where a rooted TV's SSH server is expected (Homebrew Channel bundles Dropbear). | S, D | [dev-manager] `src-tauri/src/device_manager/manager.rs:129-141`; [openlgtv-notes] |
| So an open 9922 means Developer Mode is on, and an open 9991 that its key can be fetched now. That 9922 is closed on a retail TV follows from who starts the server; no source states it. | I (from the rows above) | |
| Developer Mode lasts "a limited time"; when the session ends the apps installed with it are removed. | D | [lg-devmode] |
| Port 9998 is the inspector of apps installed in Developer Mode, but it was also open on a 2016 TV whose owner mentions no Developer Mode. It is not a signal. | D, R | [openlgtv-notes]; [h4ck] `README.md:38-41` |
| Ports 3000 and 3001 say "an LG TV that is on": `GET http://<ip>:3000/` answered `Hello world` on a 43UH6100 (webOS 3.0). On webOS 26 port 3000 resets a WebSocket handshake. | R | [h4ck] `README.md:50-54`; [aiowebostv-718] |

## Open questions

- Whether `UDN` and `deviceUUID` are equal on every webOS version, and whether
  they survive a factory reset or a firmware update.
- The `SERVER` strings and description fields of TVs older than 2018 and newer
  than 2024, and of other brands' webOS TVs.
- What LG TVs advertise over mDNS, and whether any TXT record names the model.
- Whether a TV in standby (Quick Start+, Always Ready) answers SSDP or port 3001.
- Whether port 9922 is closed on every TV without Developer Mode, and how fast
  it closes when the session expires.
- The HTTP calls on port 3000 that one client tries.
- The model year of an LCD model number (UR, UT, QNED series).
- Whether `hello` and `getSystemInfo` stay open to unpaired clients on future
  firmware: LG tightened pairing in webOS 26.

## What this means for react-native-webos

- `src/discovery/ssdp.ts` searches `urn:lge-com:service:webos-second-screen:1`,
  twice in the window. It keeps `LOCATION`, the UUID of `USN` and `SERVER`, and
  recognises a TV by the search target it answered, not by a `SERVER` string.
- `src/discovery/upnp.ts` also reads `modelNumber` and `UDN`. The model is
  `modelNumber`; `modelName` is always `LG Smart TV`. A TV is one whose
  `manufacturer` starts with `LG` and whose description has the second-screen
  service. The name still goes through `unescapeName`.
- `samsung-api.ts` has no HTTP equivalent. Its place is taken by an unpaired
  WebSocket to `wss://<ip>:3001` (then `ws://<ip>:3000`) that sends `hello` and
  `getSystemInfo` and closes: release version, `deviceUUID` and model, with no
  prompt. It also identifies a TV the sweep found when multicast is blocked. The
  sweep's ports become 3001, 3000 and 9922.
- `developerMode` is a TCP connect to 9922, and "key server on" one to 9991.
  There is no host-PC address to compare with, unlike Samsung.
- The id shown and stored is the second-screen UUID. The remembered-TVs file and
  the remote client key are keyed by it, with the last address and, once paired,
  both MAC addresses.
- The webOS version shown is `deviceOSReleaseVersion`, with its marketing name
  from the major number (7 to 11 are webOS 22 to 26). The model year is shown
  only when the model number or `model_name` gives it.
- `getCurrentSWInformation` is optional: on webOS 26 an unsigned client cannot
  read it.
- The serial number that `getSystemInfo` returns is not printed or stored.
- All of it is parsed with zod behind a size limit: the SSDP reply, the
  description (32 KiB today), the `hello` and `getSystemInfo` payloads.

## Sources

[connect-sdk]: https://github.com/ConnectSDK/Connect-SDK-Android-Core/tree/2f7ebbc7eb909321ff41b2c6bcdf6b0881427579/src/com/connectsdk
[connect-sdk-tests]: https://github.com/ConnectSDK/Connect-SDK-Android-Core/blob/2f7ebbc7eb909321ff41b2c6bcdf6b0881427579/test/src/com/connectsdk/discovery/provider/ssdp/SSDPDeviceTest.java
[connect-sdk-ios]: https://github.com/ConnectSDK/Connect-SDK-iOS-Core/blob/8ea9070b71779c8e6714fa820a9fdb140f89ce03/ConnectSDKTests/Services/Helpers/WebOSTVServiceSocketClientTests.m
[connect-sdk-windows]: https://github.com/ConnectSDK/Connect-SDK-Windows/blob/c8c53beddfa25713057fdb6fb72f17e908fa18d0/Fakes/MessageFakeFactory.cs
[ha-core]: https://github.com/home-assistant/core/tree/800f6ce08f36e90e0534f22d6b2170bf115c3405
[aiowebostv]: https://github.com/home-assistant-libs/aiowebostv/tree/f24d86bac901f67b61d6cf397681505f27b0c025/aiowebostv
[aiowebostv-tests]: https://github.com/home-assistant-libs/aiowebostv/tree/f24d86bac901f67b61d6cf397681505f27b0c025/tests
[bscpylgtv]: https://github.com/chros73/bscpylgtv/tree/191e8957a164a7a15b0b687071b534394a161aed
[lgtv2]: https://github.com/hobbyquaker/lgtv2/tree/80ef796846730cb4331f33be38221de7485afc55
[lgwebosremote]: https://github.com/klattimer/LGWebOSRemote/tree/d2923a94949a5af30c058b7dd9e2464b11ce67a0
[homebridge-webos-tv]: https://github.com/merdok/homebridge-webos-tv/tree/d5590099f8a6e8276d348a7bd4953259a5135425
[lgtvremote-cli]: https://github.com/griches/lgtvremote-cli/tree/5770baf506637e635d9381439cbcb48b7257c261
[smartthings-lgtv]: https://github.com/toddaustin07/LGTV/tree/f3c36c67848cb0358131226af08727e9793f4e4f
[nitpick]: https://github.com/JonathanSeriesX/nitpick/blob/e4a67dc97f20f300f0b16dc4bf45fefd87dd632d/Packages/NitpickKit/Tests/LGControlTests/DiscoveryTests.swift
[dev-manager]: https://github.com/webosbrew/dev-manager-desktop/tree/0251413b90eddbba64cee0b59f09070567d2a501
[h4ck]: https://github.com/chorankates/h4ck/blob/7f2e0aee15d9985fd9819af41362749a5ebbe94d/lg_webOS/README.md
[netdisco]: https://sources.debian.org/data/main/p/python-netdisco/2.8.2-1/netdisco/discoverables/webos_tv.py
[go-lgtv]: https://github.com/sweharris/go-lgtv
[lgtv-menubar]: https://github.com/sogogi445/lgtv-menubar
[gladys]: https://gladysassistant.com/docs/integrations/external/lg-webos/
[dvdremote-8]: https://github.com/dangdvd/DVDRemote/pull/8
[openlgtv-notes]: https://gist.github.com/Informatic/1983f2e501444cf1cbd182e50820d6c1
[openhab-60640]: https://community.openhab.org/t/snapshot-2-5-0-1468-lg-webos-does-not-discover-tv/60640
[openhab-14309]: https://github.com/openhab/openhab-addons/issues/14309
[ha-172703]: https://github.com/home-assistant/core/issues/172703
[ha-178411]: https://github.com/home-assistant/core/issues/178411
[ha-183602]: https://github.com/home-assistant/core/issues/183602
[aiowebostv-718]: https://github.com/home-assistant-libs/aiowebostv/issues/718
[aiowebostv-720]: https://github.com/home-assistant-libs/aiowebostv/issues/720
[lg-cli]: https://webostv.developer.lge.com/develop/tools/cli-dev-guide
[lg-devmode]: https://webostv.developer.lge.com/develop/getting-started/developer-mode-app
[snapone-ip-control]: https://help.snapone.com/sb-v4-ig/Content/Topics/IP%20Control%20Guide.htm
