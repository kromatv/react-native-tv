# Network discovery on Tizen

What a Tizen app can use to find a service on the local network (mDNS and
DNS-SD over UDP), and what is still unknown. Read it before writing discovery
code, such as KROMA's `ServerDiscovery` module
([`clients/tv-native/modules/server-discovery`](https://github.com/kromatv/kroma/tree/1d270635adcc9f3a81283e04f06336845d8c5985/clients/tv-native/modules/server-discovery)).

Tags: **D** documented, **S** read in source, **I** inferred (reason given),
**V** verified (where and when).

## Facts

| Fact | Tag | Source |
|------|-----|--------|
| Tizen has a native DNS-SD API: `dnssd_initialize`, `dnssd_start_browsing_service(service_type, &browser, found_cb, user_data)`, `dnssd_service_get_ip`, `dnssd_service_get_port`, since Tizen 3.0, privilege `http://tizen.org/privilege/internet` | D | `nsd/dns-sd.h:117,372-442,498-533`, Tizen 9.0 rootstrap |
| `libnsd-dns-sd.so` and `/usr/include/nsd/` are in Tizen 9.0's public native API, so `target_link_tizen_libraries(... nsd-dns-sd)` is allowed | D | `platforms/tizen-9.0/tizen/rootstraps/info/tizen-9.0-device.core.dev.xml:80,282` |
| DNS-SD needs the feature `http://tizen.org/feature/network.service_discovery.dnssd`; a device without it answers `DNSSD_ERROR_NOT_SUPPORTED` | D | [Network Service Discovery guide](https://docs.tizen.org/application/native/guides/connectivity/nsd); `dns-sd.h:41` |
| Samsung's TizenFX table lists `Tizen.Network.Nsd`, the .NET face of the same API, as supported on TV and on the TV emulator | D | [TizenFX API references for TV](https://www.bada.com/tv/tizen-net-tv/api-references/tizenfx-api-references/) (a mirror of Samsung's page; not re-read on developer.samsung.com) |
| The SDK's TV feature lists (`platforms/tizen-9.0/tv*/features/feature-wrt.properties`) are for web apps and do not name the DNS-SD feature | D | those files |
| A one-shot mDNS query is a plain DNS query sent to 224.0.0.251:5353 from a port other than 5353 | D | [RFC 6762, section 5.1](https://www.rfc-editor.org/rfc/rfc6762#section-5.1) |
| A responder that receives a query from a port other than 5353 "MUST send a UDP response directly back to the querier, via unicast, to the query packet's source IP address and port", repeating the query ID and question | D | [RFC 6762, section 6.7](https://www.rfc-editor.org/rfc/rfc6762#section-6.7) |
| With a service instance PTR record, a responder SHOULD add the SRV and TXT records it names and the A and AAAA records of the SRV target | D | [RFC 6763, section 12.1](https://www.rfc-editor.org/rfc/rfc6763#section-12.1) |
| KROMA's server advertises with `mdns-sd` 0.21.6, which answers a query from a port other than 5353 by unicast and puts SRV, TXT and address records beside a PTR answer | S | `mdns-sd-0.21.6/src/service_daemon.rs:3644,3772-3790` |
| No Tizen or Samsung document found states whether a native app may send to a multicast group or receive unicast UDP; the host already opens TCP sockets through libcurl under `internet` | I | searched docs.tizen.org and developer.samsung.com, 2026-11-03 |
| The simulator VM sits on Lima's user-mode network (VM 192.168.5.15/24, the Mac at 192.168.5.2): multicast neither leaves nor enters it, so a server on the Mac's LAN does not answer | V | simulator, 2026-11-03: no answer from the LAN; a responder started inside the VM answered every query |
| The simulator has no `nsd-dns-sd`: Ubuntu does not ship Tizen's library and `simulator/shims` has no stand-in | I | `simulator/README.md`, Limits and shims |

## What this means

- `ServerDiscovery` sends RFC 6762 one-shot queries over a POSIX UDP socket and
  reads the unicast answers: no Tizen library, so the simulator and a TV run the
  same code, and KROMA's advertiser answers it (verified on macOS against
  `mdns-sd` on the LAN, and in the simulator against a responder in the VM).
- Whether a retail TV lets the query out and the answer in is not known: record
  it on hardware with the model, year and Tizen version. If it does not,
  `dnssd_start_browsing_service` is the documented route, behind the feature
  check, and needs a shim before the simulator can link it.
