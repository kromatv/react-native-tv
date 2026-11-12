# The `webos` API

`@kromatv/react-native-webos/webos` is what the `rn-webos` CLI does to a TV, as
a typed Node API: find LG TVs, connect one in Developer Mode, install, launch,
close, remove and list apps, and press remote keys. It needs Node 22.13 or
newer and nothing from LG: no `ares-*` commands, no `~/.webos`.

```ts
import {
  connectTv,
  installApp,
  launchApp,
  resolveDevice,
  scanForTvs,
} from '@kromatv/react-native-webos/webos';
```

Every function that fails for a reason the user can act on throws a
`FixableError`: `message` says what happened, `hint` what to do. The TV's own
words, such as an installer's reason, are in the message.

## Find TVs

```ts
const tvs = await scanForTvs({ onProgress: (done, total) => {} });
// [{ host: '192.168.1.50', name: '[LG] webOS TV OLED65C25LB', model: 'OLED65C25LB',
//    id: '6f2c...', webosVersion: '11.2.0', developerMode: true, keyServer: false, sshPort: 9922 }]
```

`scanForTvs` sends an SSDP search for LG's remote control service and sweeps
the local `/24`s on the remote control ports (3001, 3000) and the Developer
Mode SSH port (9922). It asks each TV for its webOS release, model and id over
the remote control service, which needs no pairing. `developerMode` is whether
port 9922 is open, `keyServer` whether port 9991 is. `webosVersion` is null for
a TV whose remote control service did not answer. `hosts` limits the scan to
the addresses given.

## Connect a TV

```ts
const device = await connectTv({ host: '192.168.1.50', passphrase: 'A1b2C3', name: 'lounge' });
// { address: { host: '192.168.1.50', port: 9922 }, name: 'lounge',
//   info: { modelName: 'OLED65C25LB', sdkVersion: '11.2.0', firmwareVersion: '43.21.74' } }
```

`connectTv` needs Developer Mode on, Key Server on, and the passphrase the
Developer Mode app shows. It fetches the TV's SSH key, opens it, logs in once
and keeps the key and the TV's name in `rn-webos`'s state directory, where the
CLI finds them too. `name` defaults to the name the TV already has, else its
model. A wrong passphrase throws before anything is kept.

Once connected:

- `resolveDevice(target?)`: the connected TV with this IP address or name, or,
  with no argument, the only connected TV that answers. It logs in to read
  `info`, and throws when the TV is not connected, off, or out of Developer
  Mode.
- `listDevices()`: every connected TV, each `online` with its `device` or
  `offline` with a `reason` and a `hint`.
- `readKnownDevices()`: the connected TVs as stored (`name`, `host`, `port`),
  most recent first, without reaching them.
- `probeDevice(known)`: logs in to one of those and reads `info`.

## Install and run

```ts
const device = await resolveDevice('lounge');
await installApp(device, 'com.example.myapp_1.0.0_arm.ipk', (detail) => console.log(detail));
await launchApp(device, 'com.example.myapp');
```

- `installApp(device, file, onProgress?)`: pushes the `.ipk` to the TV, checks
  it arrived whole, installs it and removes the temporary file. It resolves
  when the TV's installer says `installed` and throws with the installer's
  reason otherwise. `onProgress` hears `sending 40%`, then the installer's
  states (`ipk verifying`, `installing`, ...). An install replaces an earlier
  one of the same app.
- `launchApp(device, appId)`: resolves when the TV accepts the launch, which
  is not yet the app drawing.
- `closeApp(device, appId)`: closes a running app installed in Developer Mode.
- `uninstallApp(device, appId)`: removes the app and its data.
- `listInstalledApps(device)`: `[{ id, version, title, type }]`, the apps
  installed in Developer Mode that the TV shows in its launcher.
- `listRunningApps(device)`: the ids of the Developer Mode apps running.

`device` is what `connectTv` and `resolveDevice` return; only its `address` is
read. Each call opens its own SSH connection and closes it. An app id is
checked against LG's rule (lowercase letters, digits, `.`, `+`, `-`) before
anything is sent.

## Remote keys

```ts
await pressRemoteKeys('192.168.1.50', ['Down', 'Enter'], {
  onPrompt: () => console.log('Accept the request on the TV'),
});
```

`pressRemoteKeys(host, keys, options?)` presses keys through the TV's network
remote, the service LG's phone apps use. It needs neither Developer Mode nor
`connectTv`. The first time, the TV asks its viewer to accept the connection
and `onPrompt` is called; the key the TV then issues is kept in the state
directory under the TV's own id. Keys are the CLI's names (`up`, `enter`,
`back`, `0` to `9`, ...) or the TV's button names in capitals (`HOME`,
`VOLUMEUP`). A key is pressed and released at once: the protocol cannot hold
one.

## Types and constants

`Device`, `DeviceInfo`, `DeviceStatus`, `KnownDevice`, `TvAddress`,
`ConnectRequest`, `InstalledApp`, `FoundTv`, `ScanOptions`, `PressOptions`,
and `DEVELOPER_SSH_PORT` (9922).

The facts behind each of these are in [`reference/`](reference/README.md):
`webos-device-protocol.md`, `webos-remote-control.md` and `tv-discovery.md`.
None of it has been run against a TV by this project yet: those files say what
is documented, what is read from LG's and other tools' source, and what is
still an open question.
