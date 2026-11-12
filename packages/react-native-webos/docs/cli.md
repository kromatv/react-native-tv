# The `rn-webos` CLI

`rn-webos` turns an Expo app into an LG webOS TV package (`.ipk`), installs
it on a TV and prints what it logs. Run it from the app's directory, or pass
`--project <dir>`. Every command that bundles (`bundle`, `package`, `run`,
`simulator run`) also takes `--entry <file>`, a JavaScript entry to bundle
instead of the app's `main`, such as one screen under test.

```sh
rn-webos init                            # set the app up for webOS, once
rn-webos doctor                          # is this machine ready?
rn-webos package                         # build an .ipk
rn-webos run --device 192.168.1.50      # build, install, launch, print logs
```

Every command prints one line per step. `--verbose` (`-v`) also prints what the
underlying tools print (CMake, Ninja, Metro, tar). When a
tool fails, the last lines it printed are shown with the error.

`rn-webos` talks to the TV itself: SSH on port 9922, the channel Developer
Mode opens, with the key the TV gives out. LG's `ares-*` commands are never run
and need not be installed. A package is not signed, and there is no certificate
to make. What a TV needs, once, is [`rn-webos connect`](#rn-webos-connect).

`rn-webos <command> --help` lists a command's options. In an app, run it with
the app's package manager: `npx rn-webos`, `bunx rn-webos`, `pnpm exec rn-webos`
or `yarn rn-webos`. `init` and the hints `rn-webos` prints name the one the
app's lockfile points to.

## Configuration

The webOS settings are the props of this package's config plugin in `app.json`
(or `app.config.js`, read through Expo's own config loader). `npx expo install
@kromatv/react-native-webos` adds the plugin to `expo.plugins` by itself, and
`npx rn-webos init` gives it its props. Only `package` is required.

```json
{
  "expo": {
    "name": "My App",
    "version": "1.2.0",
    "icon": "./assets/icon.png",
    "plugins": [
      [
        "@kromatv/react-native-webos",
        { "package": "com.example.myapp", "title": "My App", "locales": ["en", "fr"] }
      ]
    ]
  }
}
```

The plugin checks its props whenever Expo reads the config (`npx expo config`,
`npx expo start`), so a mistake shows there too. It changes nothing in the
config: Expo generates no webOS project.

| Field | Default | Meaning |
|---|---|---|
| `package` | (required) | The app id, `id` in `appinfo.json`, and the package's name: lowercase letters, digits, periods and minus signs, two characters or more, starting with a letter or a digit. Not one starting with `com.palm`, `com.webos`, `com.lge` or `com.palmdts`, which LG keeps for its own apps. |
| `title` | `expo.name` | The name the TV shows, 20 characters or fewer. Set it when `expo.name` is longer. |
| `version` | `expo.version`, else `1.0.0` | The app version: three numbers `x.y.z`, none with a leading zero. Set it when `expo.version` has a suffix such as `-beta`. |
| `vendor` | none | The app's owner, as `appinfo.json` names it. |
| `icon` | `expo.icon`, else a plain square | A PNG, relative to the app, not interlaced. The package's 80x80 `icon.png` and 130x130 `largeIcon.png` are scaled from it; one that is not square is centered on transparency. |
| `iconColor` | the TV's (white) | The color behind the icon on the launcher, as `appinfo.json` takes it, such as `#202020`. |
| `splash` | none | A 1920x1080 PNG, relative to the app, shown while the app loads (`splashBackground`). Another size is refused. |
| `permissions` | none | ACG groups of the Luna calls the app's own native code makes, such as `network.query`. They join the five the runtime always declares in `requiredACG`: `lunabus.query`, `application.launcher`, `settings.query`, `systemconfig.query` and `tts.operation`. webOS TV 26 Re:New refuses a call outside the declared groups. |
| `locales` | `["en"]` | The locales `Intl` formats in and text is broken into lines for, as BCP 47 tags (`fr`, `pt-BR`). Each adds its ICU data (by language) to the package and FormatJS's data to the bundle; any other locale falls back to the device's, then to the first listed. Japanese, Chinese, Thai, Lao, Khmer and Burmese add their word dictionary, 2 MB for the first two. |

An unknown key is an error, so a typo does not pass silently.

### The Metro config

Metro learns the `webos` platform from `withTV`, which wraps the app's Metro
config. A config plugin cannot do it: Expo's Metro setup has no hook for a
package. `rn-webos init` writes this `metro.config.mjs`, which Expo loads as an
ES module:

```js
import { getDefaultConfig } from 'expo/metro-config';
import { withTV } from '@kromatv/react-native-webos/metro';
export default withTV(getDefaultConfig(import.meta.dirname));
```

`withTV` adds every TV platform the app depends on: `webos`, and the platform
of each other dependency that declares one, such as `tizen` from
`@kromatv/react-native-tizen`. An app on both keeps this one call, imported
from either package, and a platform added later needs no change here.
`withWebOS`, from the same module, adds `webos` alone.

A package is a TV platform for `withTV` when its `package.json` names its Metro
entry, a subpath of its own `exports`, and that entry exports the wrapper as
`withTvPlatform`. The package must also export its `package.json`:

```json
{ "reactNativeTvPlatform": { "metro": "./metro", "platform": "webos", "cli": "rn-webos" } }
```

`platform` and `cli` are for a package that groups TV platforms: they name the
platform, and the command of this package's `bin` whose `run` puts the app on a
TV, so that package can offer `run:webos` beside the `run` of every other
platform. `withTV` reads `metro` alone.

An app with its own Metro config wraps its export the same way. `bundle`,
`package`, `run`, `simulator run`, and Run on TV and Run in simulator under
shift+m, load the config as `expo start` does before anything else. When it
does not know the `webos` platform, they stop and say to run
`npx rn-webos init`.

A config plugin can also hand Expo a Metro config that has the platform, by
naming it in `EXPO_OVERRIDE_METRO_CONFIG` while Expo evaluates the app config.
The app's own Metro config then needs no wrapper, and these commands accept
it, since they load the config as Expo does: the app config first, then the
Metro config Expo would take.

## Where things go

| What | Where |
|---|---|
| The generated app directory | `webos/app/` in the app: `appinfo.json`, the `react-native-webos` executable, the icons, `res/` and an empty `appdata/`. `webos/` ignores itself (`webos/.gitignore` is `*`), so it never reaches git. Delete it at any time. A `webos/` that holds files and does not ignore itself is the app's own: `rn-webos` refuses to build over it. |
| The bundle | `webos/build/bundle/` (`index.webos.js`, `index.webos.hbc`, `assets/`) |
| The package | `webos/<package>_<version>_arm.ipk` |
| Third-party licences | `res/THIRD_PARTY_NOTICES.md` in every package |
| The native runtime | `~/.cache/react-native-webos/runtime/<react-native>-<version>-<flavour>-<key>/`, the key a hash of where this package's `runtime/` and the app's react-native are, so two checkouts or two apps never share a CMake build |
| Hermes host compilers | `~/.cache/react-native-webos/hermes/<version>/` |
| The SDK TV builds compile against | `~/.cache/react-native-webos/sdk/<version>/`, 470 MB, extracted from the archive in `downloads/webos-sdk/` (360 MB) |
| ICU data, full and trimmed | `~/.cache/react-native-webos/icu/<version>/` |
| Source archives and the CA bundle | `~/.cache/react-native-webos/downloads/`, shared with CMake |
| TVs connected before | `~/.local/state/react-native-webos/devices.json`: each TV's name, address and SSH port, most recent first |
| The SSH key of each TV | `~/.local/state/react-native-webos/keys/<ip>_<port>.pem`, readable by you only: the key `connect` fetched, without its passphrase. Nothing is written to LG's `~/.webos` or to `~/.ssh` |
| Remote pairing keys | `~/.local/state/react-native-webos/remote/<tv id>.key`, readable by you only, out of the cache so a cache wipe does not unpair. The id is the TV's own, so the pairing survives a new address |

`RN_WEBOS_CACHE_DIR` moves the cache; `XDG_CACHE_HOME` is honoured too, and
`XDG_STATE_HOME` moves the keys and the TV list.
`WEBOS_LLVM_DIR` points at the directory of a clang and lld that are not
Homebrew's.

## Targets

Every build is the same: one 32-bit ARM executable (`armv7-a`, softfp, NEON),
which is what every webOS TV runs apps as, whatever its kernel. It is linked
against glibc 2.12.2, older than any TV's, with libstdc++ linked in, so one
package installs on webOS TV 4.x (2018 models) and every later release. There
is no `--arch` and no `--platform`. The facts are in
[webos-native-toolchain.md](reference/webos-native-toolchain.md).

## Inside `npx expo start`

The package is also an Expo dev tools plugin (`expo-module.config.json`), which
adds two keys to `npx expo start`, beside Expo's `a` and `i`:

| Key | Does |
|---|---|
| **t** | Opens the app on the TV used last. With no TV connected yet, it looks for TVs in Developer Mode on the network and says which to `rn-webos connect` |
| **shift+t** | Lists the TVs connected before and the simulator, and opens the app on the one whose number you press; Esc cancels |

In an app on several TV platforms, each platform package would add its own
`t`. A package that groups them takes the keys over instead: this package
registers what it can open, the TVs connected before and the simulator, under
`Symbol.for('reactNativeTvPlatform.start')` on `globalThis`, and adds no key of
its own once `claimed` is set there by the time `expo start` reads keys.

`expo start` prints `› Press t │ open on webOS TV` under its own keys, and both
lines in the list `?` shows. Opening does what Run on TV does below, with this
dev server: Expo's terminal shows the build's progress, then the app's
`console.log` and errors, as for Android. A key typed into one of Expo's own
prompts does nothing here. This needs no setup and works the same whatever
runs `expo start`: npm, pnpm, Yarn or Bun.

**shift+m** ("more tools") lists `@kromatv/react-native-webos`, with these
commands:

| Command | Runs |
|---|---|
| Open `@kromatv/react-native-webos` | The plugin's page, below |
| Run on TV | `rn-webos run --device <ip or name> --dev-server <this dev server>`, then the app's logs. Leave the address empty for the TV used last |
| Run in simulator | `rn-webos simulator run --dev-server <this dev server>`, on the instance `$RN_WEBOS_SIMULATOR_INSTANCE` names, else `default` |
| Find TVs | `rn-webos devices` |
| Connect TV | `rn-webos connect <ip>`, with the passphrase in `RN_WEBOS_PASSPHRASE` so it reaches no file: the TV's address and the passphrase its Developer Mode app shows, with Key Server on |

Expo stops a command after ten seconds, so each one starts its work in the
background and prints the address of the plugin's page,
`http://<dev server>/_expo/plugins/@kromatv/react-native-webos/`, where every
job's progress and output stream, and a running job can be stopped. Starting
Run on TV or Run in simulator again stops the previous run of the same
command. Both refuse to start, with the fix, when the app loads more than one
`react-native` or its Metro config does not know `webos`. The jobs are kept in `~/.cache/react-native-webos/devtools/jobs/`, the
twenty most recent.

A development build loads its JavaScript from the dev server, and needs React
Native's development support in the runtime (`DevSettings`, Fast Refresh) to
start from it.

Expo does not know the webOS platform beyond that: there is no
`expo run:webos`, and neither `expo prebuild` nor
`expo export --platform webos` takes it (`expo export` refuses a platform it
does not bundle, which is why `rn-webos bundle` goes through `expo
export:embed`). A `package.json` script stands beside Expo's own:

```json
{
  "scripts": {
    "android": "expo run:android",
    "webos": "rn-webos run"
  }
}
```

## Commands

### `rn-webos init`

Sets an Expo app up for webOS. Running it again changes nothing.

- **The config plugin.** It lists the plugin in `app.json` with the app's
  id, or gives the entry `npx expo install` added by name its props. On a
  terminal it asks for the id, suggesting the app's `android.package` when
  webOS takes it (lowercase, no underscore), else `com.anonymous.<slug>` as
  `expo prebuild` does; without one it takes the suggestion. Expo evaluates an
  `app.config.*` file, so `init` prints the entry to add to it instead.
- **The Metro config.** It writes `metro.config.mjs` when the app has none. An
  existing config whose export is a single line is wrapped with `withTV`, and
  one that already calls `withTV` or `withWebOS` is left as it is.
  For any other, a JSON one, `package.json`'s `metro` key or a config at the
  workspace root above the app, it prints the two lines to add.
- **React Native.** It points the app's `react-native` dependency at
  `npm:react-native-tvos@0.88.0-0`, unless it names a react-native-tvos
  already, and overrides every other `react-native` in the tree with it, where
  the package manager of the nearest lockfile reads overrides: `overrides` in
  `package.json` for npm and Bun, `resolutions` for Yarn, `pnpm.overrides` for
  pnpm 10 and older, `overrides` in `pnpm-workspace.yaml` for pnpm 11 and later.
  Without the override, npm installs a second, upstream `react-native` for
  react-native-tvos's own `@react-native-tvos/virtualized-lists`. It then runs
  the package manager's install when it changed anything or the app loads more
  than one `react-native`, and fails, naming each copy and who loads it, unless
  one is left.

| Option | |
|---|---|
| `--package <id>` | The app id, instead of asking |
| `--no-install` | Change the files without running the install; `init` prints the command |
| `--project <dir>` | The app to set up |

```sh
npx rn-webos init
npx rn-webos init --package com.example.myapp
```

### `rn-webos doctor`

Checks everything a build and an install need, and prints the exact fix for
each problem: the SDK TV builds compile against (a warning until the first
build or `rn-webos setup` downloaded it), Homebrew's LLVM (clang and lld),
CMake 3.24 or newer, Ninja, Node.js, ccache, the TVs connected before (each one
logged in to: reachable, Developer Mode on, key still accepted), and, when run
in an app, its config and that it loads one `react-native`. Exits 1 when a
check fails; a warning (no SDK yet, no TV answers, no ccache) does not fail it.

| Option | |
|---|---|
| `--project <dir>` | The app to check |

### `rn-webos devices`

Finds the LG TVs on this machine's networks and probes each one, along with
every TV `rn-webos` was connected to before. The scan sends an SSDP search for
LG's remote control service and tries each host of the local `/24`s, the ones
this machine talked to lately first, on the remote control ports (3001, 3000)
and the Developer Mode SSH port (9922). It asks each TV what it is over its
remote control service, which needs no pairing and shows nothing on the TV. It
takes about five seconds.

For each TV it prints:

- **webOS**: the release the TV reports, with the name LG sells it under
  (`webOS 26 (11.2.0)`).
- **Developer Mode**: on when port 9922 is open.
- **Key Server**: on when port 9991 answers, which is when `connect` can fetch
  the key. It turns off whenever the TV restarts, and a connected TV does not
  need it.
- **rn-webos**: `connected as <name>` when the key `rn-webos` keeps still logs
  in, else the `connect` command to run, or why the login failed.

| Option | |
|---|---|
| `--no-scan` | Only probe the TVs connected before |

```sh
rn-webos devices
rn-webos devices --no-scan
```

### `rn-webos connect`

Sets a TV up for `run`, `install` and `uninstall`. Do it once per TV, and
again if the TV ever refuses the key.

On the TV, first:

1. Install the **Developer Mode** app from LG's store and sign in with an LG
   developer account.
2. Turn **Dev Mode Status** on. The TV restarts.
3. Open the app again and turn **Key Server** on. Note the passphrase at the
   bottom left: six characters, upper and lower case as shown.

Then:

```sh
rn-webos connect 192.168.1.50
rn-webos connect 192.168.1.50 --passphrase A1b2C3 --name living-room
```

`connect` fetches the TV's SSH key from its key server
(`http://<tv>:9991/webos_rsa`), opens it with the passphrase, logs in once to
prove it works, and keeps the key with the TV's name and address (see
[Where things go](#where-things-go)). It asks for the passphrase on a terminal;
anywhere else pass `--passphrase` or set `RN_WEBOS_PASSPHRASE`. A wrong passphrase is reported as such and
nothing is kept. Key Server can be turned off afterwards.

| Option | |
|---|---|
| `--passphrase <code>` | The six characters the Developer Mode app shows (default: `$RN_WEBOS_PASSPHRASE`, else asked for) |
| `--name <name>` | What `--device` calls this TV from now on (default: the name it already has, else its model). Letters, digits, `.`, `_` and `-` |

Every command that takes `--device, -d` takes the TV's IP address or this name.
Without it, `run`, `install` and `uninstall` use the only connected TV that
answers, and say which ones answer when there are several.

Developer Mode lasts a limited time, which the Developer Mode app shows as the
remaining session and extends. When it runs out the TV turns Developer Mode off
on its next restart and removes the apps installed with it; `rn-webos devices`
and `doctor` then say that the TV no longer accepts SSH.

### `rn-webos install`

Installs any `.ipk` on a TV and, with `--launch`, starts it: a package from
another build, or a release a CI made. It pushes the file to
`/media/developer/temp` on the TV (over SFTP, else through `cat` on a TV
without it), compares its MD5 with the one the TV computes, asks the TV's
installer for it, follows the installer until it says `installed` or fails
with its reason, and removes the temporary file either way. An install
replaces an earlier one of the same app.

`--launch` starts the app the installer read from the package, else the one
the file name gives when it follows `<app id>_<version>_<architecture>.ipk`.

| Option | |
|---|---|
| `--device, -d <ip or name>` | The TV (default: the only connected one that answers) |
| `--launch` | Launch the app once it is installed |

```sh
rn-webos install com.example.myapp_1.0.0_arm.ipk --device 192.168.1.50 --launch
```

### `rn-webos uninstall`

Removes an app and its data from a TV, by its app id.

```sh
rn-webos uninstall com.example.myapp --device 192.168.1.50
```

### `rn-webos setup`

Downloads the SDK a TV build compiles and links against, once: openlgtv's
`buildroot-nc4` 2026.08, a 360 MB archive checked against its pinned SHA-256.
It extracts the sysroot, the C++ headers, the static C++ runtime and GCC's own
files into `~/.cache/react-native-webos/sdk/`, 470 MB, and leaves the archive's
compilers alone: the build uses this machine's clang and lld. `package`, `run`
and `build-runtime` do this on their own; run it by hand only to prepare a
machine ahead of time. It takes no option but `--verbose`.

### `rn-webos build-runtime`

Compiles `react-native-webos`, the executable a package starts: Hermes, React
Native's C++ core, the host, Skia, SDL and the app's Expo modules core, against
the app's own `react-native` and `expo-modules-core`. The first build takes
several minutes; later ones only rebuild what changed. It also builds the
Hermes compilers for this machine, once per Hermes version.

Takes `--project`. It builds the development runtime, the one `run` packages,
unless given `--release`: release builds get a runtime without the dev server
tooling ([decisions](decisions.md)).

### `rn-webos bundle`

Checks that the app's Metro config knows the `webos` platform, then bundles the
app's JavaScript with `expo export:embed --platform webos`, through that config,
and compiles it to Hermes bytecode with the `hermesc` that ships with the app's
`react-native`.

| Option | |
|---|---|
| `--out <dir>` | Where to write the bundle (default `webos/build/bundle`) |
| `--entry <file>` | The entry to bundle (default: the app's `main`) |

Every bundle replaces the `assets/` folder of the directory it writes to, so
`--out` must be a new or empty directory, or one `rn-webos bundle` wrote
before (it leaves a `.rn-webos-bundle` file there). Anything else, such as
`--out .` in the app, fails before a file changes.

### `rn-webos package`

Builds an `.ipk` from scratch or incrementally: the native runtime, the bundle,
the ICU data and the CA bundle, the app directory in `webos/app/`, then the
package, `webos/<package>_<version>_arm.ipk`. It checks the app's
`react-native` and Metro config first, so a second copy of `react-native` or a
Metro config that does not know `webos` fails in seconds rather than after a build.

`rn-webos` writes the package itself, in the layout LG's `ares-package` writes
([webos-ipk-format.md](reference/webos-ipk-format.md)); `ares-package` is never
run. A webOS package is not signed. What it holds, under
`usr/palm/applications/<package>/`:

| Path | |
|---|---|
| `appinfo.json` | A native app (`type: "native"`, `main: "react-native-webos"`), from the config plugin's props. An app that depends on `expo-video` also gets `transparent: true`, which lets the TV's video plane show through the window |
| `react-native-webos` | The runtime, one 32-bit ARM executable, without its symbol table. The copy with symbols stays in `webos/build/`, where `run` and `logs` name the frames of a native crash from |
| `icon.png`, `largeIcon.png`, `splash.png` | 80x80 and 130x130, scaled from the app's icon; the splash image when the app has one |
| `res/` | `index.webos.hbc`, `assets/`, [`react-native-webos.json`](runtime-config.md), `icudt78l.dat`, `cacert.pem`, `app.config` and `THIRD_PARTY_NOTICES.md` |
| `appdata/` | Empty and world-writable: where the app keeps its data and cache on the TV, which does not let an app write elsewhere in its directory on every release |

`res/cacert.pem` is Mozilla's root certificates as curl publishes them, pinned
in `runtime/dependencies.json` and checked against its SHA-256. The runtime's
HTTPS trusts that file and not the TV's own list, which is as old as the TV's
firmware.

| Option | |
|---|---|
| `--dev` | Build for development: the app streams its logs to this computer |
| `--device, -d <ip>` | With `--dev`, the TV the build is for: the logs go to this computer's address on that TV's network |
| `--log-host <ip>` | With `--dev`, where the app streams its logs (default: this computer's LAN address, on the TV's subnet when `--device` is given) |
| `--log-port <port>` | With `--dev`, the port it streams to (default `8099`) |
| `--dev-server <origin>` | With `--dev`, load the JavaScript from this `expo start` dev server, such as `http://localhost:8081`, instead of packaging it |
| `--entry <file>` | The entry to bundle (default: the app's `main`) |

`package` makes a release build: it streams no logs and carries the runtime
without the dev server tooling. A development build (`--dev`) carries the log
endpoint in `res/react-native-webos.json`. Every package carries
`res/THIRD_PARTY_NOTICES.md`, the licences of what the runtime links and the
bundle holds ([`THIRD_PARTY_NOTICES.md`](../THIRD_PARTY_NOTICES.md)); add the
notices of the native libraries your app adds to your own.

```sh
rn-webos package
rn-webos package --dev --device 192.168.1.50
```

### `rn-webos run`

Packages a development build of the app, installs it on a TV, launches it,
then prints its logs until Ctrl+C. Takes every `package` option but `--dev`,
plus:

| Option | |
|---|---|
| `--release` | Build for the store instead: the app streams no logs, so nothing prints |
| `--level <level>` | The quietest log level to show: `debug`, `info`, `warn` or `error` (default `info`) |
| `--no-logs` | Exit once the app is launched |
| `--dev-server <origin>` | Load the JavaScript from this `expo start` dev server instead of packaging it: see [Develop with `expo start`](#develop-with-expo-start) |

`--device, -d` takes the TV's IP address or the name it was connected under;
without it, `run` uses the only connected TV that answers. A TV `rn-webos` is
not connected to is refused before anything compiles, with the
[`connect`](#rn-webos-connect) command to run. `Install` pushes the package to
the TV and installs it, replacing any earlier install, and `Launch` starts it,
both on one SSH login. Launched means the TV accepted the launch, not that the
app drew. The log listener starts before the build, so a port already in use
fails before anything compiles, and nothing the app logs at startup is lost.

```sh
rn-webos run --device 192.168.1.50
rn-webos run --level debug
rn-webos run --dev-server http://localhost:8081
```

### Develop with `expo start`

```sh
npx expo start                                   # in the app, as for Android, then press t
rn-webos run --dev-server http://localhost:8081  # or install, once, a build that loads from it
```

`t` and shift+t in `expo start`, and Run on TV and Run in simulator under
shift+m, run the same commands with the dev server's own address. A build made with `--dev-server`
carries no bundle: its runtime config names `bundleUrl`, Expo's entry
(`/.expo/.virtual-metro-entry.bundle?platform=webos&...`) on that server, with
`localhost` replaced by an address the TV reaches, and the dev server serves
the assets. The TV fetches the bundle at each start, so the dev server must be
running then. Then, as with an Android device:

- an edit reaches the TV through Fast Refresh, keeping state;
- `r` in `expo start`'s terminal, or `DevSettings.reload()`, reloads the app;
- `console.log` and errors from the TV print in that terminal;
- LogBox shows errors and warnings on the TV; Back closes its inspector;
- `j` opens React Native DevTools on the TV's Hermes: breakpoints, the
  console, and the Components tab.

Install the build once: it keeps working across edits, reloads and restarts of
`expo start`, as long as the server keeps its address and port. `rn-webos
simulator run --dev-server <origin>` does the same on the simulator. The TV still
streams its log lines and native crashes to `rn-webos`, which prints them
until Ctrl+C: Metro only sees what JavaScript logs, and nothing from the
native runtime.

### `rn-webos logs`

Prints what a development build streams, from any number of launches, until
Ctrl+C. Use it to follow an app launched from the TV's own menu.

Every development build names its package when it connects, so one port can
carry several apps on a TV. `logs`, `perf` and `screenshot` read the app
`--package` names, else the one in `--project` or the current directory, and
each connection line names its app. `run` reads the app it installed. A build
made before this release names no package and is read whatever the target.

| Option | |
|---|---|
| `--port <port>` | The port the app was packaged with (default `8099`) |
| `--level <level>` | The quietest level to show (default `info`) |
| `--package <id>` | The app to read, by package id (default: the app in `--project` or the current directory, else the only one connected) |
| `--project <dir>` | The app whose package to read |

### `rn-webos key`

Presses remote keys on a TV, in order, through the TV's network remote: the
service LG's phone apps use, on port 3001 (3000 on older TVs)
([webos-remote-control.md](reference/webos-remote-control.md)). The app in
front can be any app, not only a development build, and the TV needs neither
Developer Mode nor `connect`. Its Network settings must allow LG Connect Apps.

```sh
rn-webos key Down Down Enter --device 192.168.1.50
rn-webos key Back
rn-webos key VOLUMEUP --device living-room
```

A key is a name (`up`, `down`, `left`, `right`, `enter`, `back`, `exit`, `info`,
`play`, `pause`, `stop`, `rewind`, `fastforward`, the four colors, `0` to `9`,
in any case) or any of the TV's button names in capitals, as written: `HOME`,
`MENU`, `VOLUMEUP`, `CHANNELDOWN`. An LG TV has no play/pause toggle, so
`playpause` is refused: send `play` or `pause`.

**A key is pressed and released at once. The protocol has no way to hold one**,
so a long press cannot be sent to a TV this way.

The first time, the TV shows a connection request: accept it with the remote.
The key the TV then issues is saved (see [Where things go](#where-things-go)),
so later presses need nobody at the TV.

| Option | |
|---|---|
| `--device, -d <ip or name>` | The TV (default: the TV connected last) |

### `rn-webos screenshot`

Saves what a development build draws as a PNG: the app captures its own window
and sends it over its log connection, so it needs nothing from the TV, whose
own screen capture is closed to a Developer Mode login. It shows the app's UI
only, never the video plane. It
goes through the `run` or `logs` already holding the log port, else listens on
it until the app connects.

| Option | |
|---|---|
| `--device, -d <ip or name>` | The TV whose app to capture (default: the only one connected) |
| `--out <file.png>` | Where to save it (default `tv/<ip>/screenshot.png` in the cache) |
| `--port <port>` | The port the app was packaged with (default `8099`) |
| `--package <id>` | The app to read, by package id (default: the app in `--project` or the current directory, else the only one connected) |
| `--project <dir>` | The app whose package to read |

With no target and several apps connected, it names them and asks for
`--package` instead of guessing.

### `rn-webos perf`

Prints, once a second, where a development build's time goes: the frames the
main loop drew and dropped, their render times, how busy the main loop and the
JavaScript thread were, Fabric's commits and mounts, the Hermes heap, the app
process's memory and the CPU. At Ctrl+C, or after `--duration`, it prints the
run's average, 95th percentile and worst of each, and the run's highest
memory. The memory is the process's PSS and swap, read on the TV from
`/proc/self/smaps_rollup`; GPU memory, which an app cannot read about itself,
comes on top, so treat the figure as a floor. LG publishes no memory budget
for an app, so the figure is compared with nothing. It listens on the
log port, as `logs` does, and asks the app for samples: an app measures nothing
until asked, and stops when `perf` exits. A release build has none of it.
Samples come from one app: the one `--package` names, else the app in the
current directory, else the first that connects, and `perf` names any other
app that connects too.

```sh
rn-webos run --device 192.168.1.50 --no-logs
rn-webos perf --device 192.168.1.50
rn-webos perf --port 8102 --duration 20 --out boot.jsonl
```

To measure a cold start, start `perf` first, then launch the app. Since
`perf` holds the log port, launch with `run --no-logs`; log lines at
`--level` or louder print between the samples.

| Option | |
|---|---|
| `--device, -d <ip>` | Only read the app on this TV (default: any that connects) |
| `--port <port>` | The port the app was packaged with (default `8099`) |
| `--duration <seconds>` | Stop this long after the first sample and print the summary (default: at Ctrl+C) |
| `--out <file.jsonl>` | Also save every sample, one JSON object per line |
| `--level <level>` | The quietest log level to print between samples (default `warn`) |
| `--package <id>` | The app to read, by package id (default: the app in `--project` or the current directory, else the only one connected) |
| `--project <dir>` | The app whose package to read |

Each line reads:

```
09:00:05  fps  58/60  drop   2  render   3.1/   9.4 ms  main  21%  js  35%  wait   4%  lag    2.0 ms  commit   3x  12.1/  30.2 ms  mount   4.0/   9.0 ms  heap  12.5/   32 MB  mem   142 MB  cpu  45%
```

A second in which nothing drew, committed or ran prints `idle`. The fields,
as `--out` saves them (times in milliseconds):

| Field | Meaning |
|---|---|
| `app` | The package the sample came from (null for a build that names none) |
| `t`, `window` | When the sample ended, since the app started, and how long it covers (about 1000) |
| `frames` | Frames the scene drew and presented |
| `renderAvg`, `renderMax` | Time to draw a frame with Skia and hand it to the GPU, per frame; the swap that follows is not in it |
| `ticks`, `hz` | Frame ticks of the main loop while it was free, and the display rate their median interval gives |
| `dropped` | Display frames the main loop missed while the scene had something to draw |
| `mainBusy` | Time the main loop spent awake: input, mounting, animation and drawing, without its wait in the swap |
| `jsBusy` | Time the JavaScript thread ran tasks |
| `jsWait` | Time a worker, the JavaScript thread measuring text above all, waited for the main loop |
| `jsLag` | How long the last task queued on the JavaScript thread waited before it ran |
| `commits`, `commitAvg`, `commitMax`, `layoutMax` | Fabric commits mounted in the window, and the commit (with its layout) time on the JavaScript thread |
| `mutations` | View mutations mounted |
| `mountAvg`, `mountMax` | Diff and mount time on the main loop, per commit |
| `mountLatencyMax` | The longest wait between a commit ending and the main loop mounting it |
| `heapUsedMb`, `heapSizeMb`, `gcs` | Hermes's `getHeapInfo`: allocated bytes, heap size, collections so far |
| `pssMb`, `swapMb`, `memorySource` | The process's PSS and proportional swap from `/proc/self/smaps_rollup` (`memorySource: "smaps_rollup"`), or its RSS and swap from `/proc/self/status` on a kernel without that file (`"status"`, shared pages counted in full). `mem` on each line is their sum, red past the FHD budget |
| `cpu` | The process's CPU time, in percent of one core |

### `rn-webos simulator`

Runs the app on a virtual TV on this Mac (Apple silicon, with
[Lima](https://lima-vm.io)), headless or with a live screen in the browser, and
drives it. The first run creates the VM and compiles React Native for it;
later runs rebuild only what changed.

```sh
rn-webos simulator run --gui           # build, bundle, start, open the live screen
rn-webos simulator run --dev-server http://localhost:8081  # load the bundle from `expo start` instead
rn-webos simulator key Down Enter      # press remote keys
rn-webos simulator screenshot --out shot.png
rn-webos simulator crash               # backtraces of the last native crash
rn-webos simulator perf --duration 20  # frame times, thread load and heap
rn-webos simulator run --instance ui-a  # a second app on its own screen
rn-webos simulator run --entry src/screens/svg/main.ts  # one screen
rn-webos simulator relaunch --params '{"contentTarget":"myapp://item/42"}'  # launch it again, as the TV does with a deep link
rn-webos simulator restart --screen 1280x720  # the screen a Full HD TV gives an app
rn-webos simulator clean               # delete the runtime builds no instance runs
```

The VM holds any number of **instances**, each with its own screen, app, live
screen and log port, so several people or agents use it at once. Every action
takes `--instance <name>`, else `$RN_WEBOS_SIMULATOR_INSTANCE`, else `default`;
`status` lists every instance with its display and ports.

The runtime compiles the app's native libraries in, so the VM keeps one
runtime build per set of them: apps with the same libraries share one, and
each instance runs the build it was last staged on. The VM keeps the four most
recently used; `rn-webos simulator clean` deletes every build no instance runs.

`run --dev-server <origin>` loads the JavaScript from an `expo start` dev
server on the Mac, as `rn-webos run` does
([Develop with `expo start`](#develop-with-expo-start)), the VM reaching the
Mac at `host.lima.internal`.

Every action, and how the simulator works, is in
[`simulator/README.md`](../simulator/README.md).

## Logs

An LG TV does not let a Developer Mode login read the system log, so a
development build connects out to this computer and streams JSON lines (`{"level":"info","message":"..."}`) over TCP to the address
in its `react-native-webos.json`. `rn-webos` prints each one as
`HH:MM:SS level message`, the level colored, a stack trace indented under its
message. The TV must reach this computer on that port: allow incoming
connections if a firewall asks.

A log line is untrusted input. `rn-webos` drops its control characters, all
but line feed and tab, before printing it, so an app cannot move the cursor or
retitle the terminal. When it knows the TV (`run`, and `perf` or `screenshot`
with `--device`), it closes any connection from another host but this
computer; `logs` and the commands without `--device` take any host on the
network.

While nothing listens, the app tries again every 500 ms and keeps its last 200
lines at `info` or louder. The next listener gets those first, after a line
saying how many it missed. Start `logs` or `perf` at any time: the app finds
it within half a second.

## Exit codes

| Code | |
|---|---|
| `0` | Done |
| `1` | A step failed, or `doctor` found a problem |
| `2` | The command line was wrong: unknown command or option, bad value |
