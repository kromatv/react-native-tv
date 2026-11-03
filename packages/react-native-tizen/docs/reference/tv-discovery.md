# Finding Samsung TVs on the network

How `rn-tizen devices` finds a Samsung TV from the computer and tells whether it
is ready for an install: SSDP, the TV's REST API, its UPnP description, the ARP
table and the sdb port. For what an app on the TV can use to find a server, see
[tizen-network.md](tizen-network.md). Checked on 2026-11-03.

| Tag | Meaning |
|---|---|
| **D** | Documented: a specification |
| **H** | Seen on hardware: GQ75LS03DAUXZG (2024 Frame, Tizen 9.0) and a HW-QS700F soundbar on the same Wi-Fi network, from an arm64 Mac |
| **O** | Observed on the Mac |

## Facts

| Fact | Tag | Source |
|---|---|---|
| An SSDP search is a multicast `M-SEARCH * HTTP/1.1` to 239.255.255.250:1900 with `MAN: "ssdp:discover"`, `MX` (1 to 5 seconds) and `ST`, the search target; a device answers by unicast with a `LOCATION` header. | D | [upnp-arch] 1.3.2, 1.3.3 |
| The TV answers `ST: urn:dial-multiscreen-org:service:dial:1` with `LOCATION: http://<tv>:7678/nservice/` and `SERVER: Samsung-Linux/4.1, UPnP/1.0, Samsung_UPnP_SDK/1.0`. It answers `upnp:rootdevice` and Samsung's own service types too; `urn:samsung.com:device:RemoteControlReceiver:1` got no answer. | H | |
| Its description at that location names `friendlyName` (`75&quot; The Frame`, HTML-escaped), `manufacturer` `Samsung` and `modelName` `GQ75LS03DAUXZG`. | H | |
| `GET http://<tv>:8001/api/v2/` answers JSON whose `device` holds `name` (HTML-escaped as above), `modelName`, `model` (`24_PTM_FTV_T09`), `OS` (`Tizen`), `type` (`Samsung SmartTV`), `developerMode` (`"1"` when on) and `developerIP` (the host PC Developer mode names). No Samsung page documenting this endpoint was found. | H | |
| The API's `device.duid` is a UUID (`uuid:...`). It is not the DUID Samsung certificates bind: that is what sdb's `0 getduid` prints ([sdb-protocol.md](sdb-protocol.md) section 4), the value a distributor certificate lists as `URI:URN:tizen:deviceid=<duid>` in its subject alternative names. | H, O | |
| A Samsung soundbar answers the same API with `type` `Samsung SmartTV` and `OS` `Tizen`; it tells itself apart by `ModelNumber` `HW-QS700F` and a `model` ending in `SPK` (`25_MT8532_AISPK`). Its sdb port is closed. | H | |
| `arp -an` on macOS lists `? (<address>) at <mac> on <interface> ...`, and `(incomplete)` for an address that never answered. | O | |
| A scan of one `/24` (SSDP for 2.5 s, then ports 8001 and 26101 on each host, 64 at a time, 500 ms per connect) found the TV and the soundbar in 4.2 s. | O | |

## What this means

- The scan asks the REST API first: it alone says whether Developer mode is on
  and which host PC it names. The UPnP description names a TV whose REST API
  is off; an open sdb port alone still counts as a TV.
- A soundbar is left out by its `HW-` model number and `SPK` model.
- The DUID shown is sdb's, read only when sdb answers, since that is the one a
  certificate must list.

## Unknowns

- Whether TVs of other model years answer the same SSDP search and REST API.
- Whether Samsung documents the REST API at all.

## Sources

[upnp-arch]: http://upnp.org/specs/arch/UPnP-arch-DeviceArchitecture-v1.1.pdf
