# The `rn-tizen` CLI

`rn-tizen` turns an Expo app into a signed Samsung TV package (`.tpk`), installs
it on a TV and prints what it logs. Run it from the app's directory, or pass
`--project <dir>`. Every command that bundles (`bundle`, `package`, `run`,
`simulator run`) also takes `--entry <file>`, a JavaScript entry to bundle
instead of the app's `main`, such as one screen under test.

```sh
rn-tizen doctor                          # is this machine ready?
rn-tizen package                         # build a signed .tpk
rn-tizen run --device 192.168.1.50      # build, install, launch, print logs
```

Every command prints one line per step. `--verbose` (`-v`) also prints what the
underlying tools print (CMake, Ninja, Metro, dotnet, the Tizen CLI). When a
tool fails, the last lines it printed are shown with the error.

`rn-tizen` talks to the TV itself, over sdb's protocol on port 26101: Tizen
Studio's `sdb` is never run. A TV takes one sdb connection at a time, so while
Tizen Studio's `sdb` or Device Manager is connected to it, `rn-tizen` cannot
reach it: `sdb disconnect <tv ip>` frees it.

`rn-tizen <command> --help` lists a command's options.

## Configuration

A Tizen build reads `expo.tizen` in `app.json` (or `app.config.js`, through
Expo's own config loader). Only `package` is required.

```json
{
  "expo": {
    "name": "My App",
    "version": "1.2.0",
    "icon": "./assets/icon.png",
    "tizen": {
      "package": "com.example.myapp",
      "label": "My App",
      "privileges": ["internet"]
    }
  }
}
```

| Field | Default | Meaning |
|---|---|---|
| `package` | (required) | The Tizen package and app id, reverse-DNS: letters, digits and underscores in each part. |
| `label` | `expo.name` | The name the TV shows. |
| `version` | `expo.version` | The package version, `x.y.z` with numbers only. Set it when `expo.version` has a suffix such as `-beta`. |
| `icon` | `expo.icon`, else a default | A PNG, relative to the app. |
| `privileges` | `["internet"]` | Tizen privileges. A short name expands to `http://tizen.org/privilege/<name>`; a full URI is kept as written. |
| `platform` | `9.0` | The oldest Tizen version the package runs on. |
| `profile` | the active one | The Tizen Studio security profile that signs the package. |
| `locales` | `["en"]` | The locales `Intl` formats in, as BCP 47 tags (`fr`, `pt-BR`). Each adds its ICU data (by language) to the package and FormatJS's data to the bundle; any other locale falls back to the device's, then to the first listed. |
| `voice` | off | `true`, or `{ "searchUrl": "myapp://search?q={query}" }`: Samsung's voice assistant. See [tv-features.md](tv-features.md#voice). |
| `preview` | off | `{ "endpoint": "<http or https URL>" }` for a public Smart Hub preview, `{ "refreshMinutes": 30 }` (10 or more) for a personal one the app publishes. See [tv-features.md](tv-features.md#smart-hub-preview). |
| `resolution` | `1080p` | `4k` asks the window manager for a 3840x2160 UI plane on a 4K panel. See [tv-features.md](tv-features.md#4k-ui). |

An unknown key is an error, so a typo does not pass silently. expo-video's
config plugin option `supportsBackgroundPlayback` is read too
([tv-features.md](tv-features.md#background-audio)).

## Where things go

| What | Where |
|---|---|
| The generated .NET project | `tizen/` in the app. It ignores itself (`tizen/.gitignore` is `*`), so it never reaches git. Delete it at any time. |
| The bundle | `tizen/build/bundle/` (`index.tizen.js`, `index.tizen.hbc`, `assets/`) |
| The signed package | `tizen/bin/Release/<target framework>/<package>-<version>.tpk`, `tizen90` for Tizen 9.0 (TizenFX API 9, not the Tizen version) |
| Third-party licences | `res/THIRD_PARTY_NOTICES.md` in every package |
| The native runtime | `~/.cache/react-native-tizen/runtime/<react-native>-<version>-<arch>-tizen<platform>-<key>/`, the key a hash of where this package's `runtime/` and the app's react-native are, so two checkouts or two apps never share a CMake build |
| Hermes host compilers | `~/.cache/react-native-tizen/hermes/<version>/` |
| Linkable sysroots | `~/.cache/react-native-tizen/sysroots/` |
| Source archives | `~/.cache/react-native-tizen/downloads/`, shared with CMake |
| Remote pairing tokens | `~/.local/state/react-native-tizen/remote/<tv>.token`, readable by you only, out of the cache so a cache wipe does not unpair; one an older `rn-tizen` left in the cache moves there on first use |
| TVs reached before | `~/.local/state/react-native-tizen/devices.json`, most recent first |

`RN_TIZEN_CACHE_DIR` moves the cache; `XDG_CACHE_HOME` is honoured too, and
`XDG_STATE_HOME` moves the tokens and the TV list.
`TIZEN_SDK_DIR` points at Tizen Studio when it is not in `~/tizen-studio`, and
`TIZEN_LLVM_DIR` at a clang and lld that are not Homebrew's.

## Targets

A build is for one CPU and one Tizen version. Each is chosen in this order:

- **CPU** (`--arch`): the flag, then what the TV reports, then `armv7l`.
- **Tizen version** (`--platform`): the flag, then `expo.tizen.platform`, then
  the newest supported version the TV can run, then `9.0`.

A package runs on its own Tizen version and every later one. Building for a
version newer than the TV's is refused, since the TV could not install it.
Only Tizen 9.0 is verified today; any other version is refused with the list of
supported ones.

## Commands

### `rn-tizen doctor`

Checks everything a build, a signature and an install need, and prints the
exact fix for each problem: Tizen Studio, its GCC runtime and rootstrap for the
target, Homebrew's LLVM (clang and lld), CMake 3.24 or newer, Ninja, the .NET
SDK, Node.js, an active security profile, the TVs reached before (each one
probed), ccache, and the app's config when run in an app. Exits 1 when a check
fails; a warning (no TV answers, no ccache) does not fail it.

| Option | |
|---|---|
| `--arch <arch>` | The CPU to check a toolchain for (default `armv7l`) |
| `--platform <version>` | The Tizen version to check a rootstrap for (default `9.0`) |
| `--project <dir>` | The app to check |

### `rn-tizen devices`

Probes every TV `rn-tizen` reached before and lists each one's model, CPU and
Tizen version, or why it did not answer. A TV is remembered once `devices
--connect`, `run` or `package` reaches it with `--device`.

| Option | |
|---|---|
| `--connect <ip>` | Connect to the TV at this address first, and remember it (port 26101 unless given) |

```sh
rn-tizen devices --connect 192.168.1.50
```

### `rn-tizen setup`

Makes a target buildable: compiles the glibc compat archive against the
rootstrap and mirrors the rootstrap into a sysroot lld can link against.
`package`, `run` and `build-runtime` do this on their own; run it by hand only
to prepare a machine ahead of time.

| Option | |
|---|---|
| `--arch <arch>` | `armv7l`, `aarch64` or `x86_64` (the emulator) |
| `--platform <version>` | The Tizen version whose rootstrap to use |

### `rn-tizen build-runtime`

Compiles `libreact-native-tizen.so` (Hermes, React Native's C++ core, the
Tizen host and the app's Expo modules core) against the app's own
`react-native` and `expo-modules-core`. The first build takes several minutes;
later ones only rebuild what changed. It also builds the Hermes compilers for
this machine, once per Hermes version.

Takes `--arch`, `--platform` and `--project`.

### `rn-tizen bundle`

Bundles the app's JavaScript with `expo export:embed --platform tizen`, through
the app's Metro config, and compiles it to Hermes bytecode with the `hermesc`
that ships with the app's `react-native`.

| Option | |
|---|---|
| `--out <dir>` | Where to write the bundle (default `tizen/build/bundle`) |
| `--entry <file>` | The entry to bundle (default: the app's `main`) |

### `rn-tizen package`

Builds a signed `.tpk` from scratch or incrementally: the native runtime, the
bundle, the `tizen/` project, the .NET build, then signing with the security
profile. It checks the profile first, so a machine without one fails in
seconds rather than after a build.

| Option | |
|---|---|
| `--device, -d <ip>` | The TV to build for; its CPU and Tizen version become the defaults |
| `--arch <arch>` | Override the CPU |
| `--platform <version>` | Override the Tizen version |
| `--profile <name>` | Sign with this security profile |
| `--dev` | Build for development: the app streams its logs to this computer |
| `--log-host <ip>` | With `--dev`, where the app streams its logs (default: this computer's LAN address, on the TV's subnet when `--device` is given) |
| `--log-port <port>` | With `--dev`, the port it streams to (default `8099`) |
| `--entry <file>` | The entry to bundle (default: the app's `main`) |

`package` makes a release build, the one Samsung's store takes: it streams no
logs. A development build (`--dev`) carries the log endpoint in
`res/react-native-tizen.json`. Every
package carries `res/THIRD_PARTY_NOTICES.md`, the licences of what the
runtime links and the bundle holds
([`THIRD_PARTY_NOTICES.md`](../THIRD_PARTY_NOTICES.md)); add the notices of
the native libraries your app adds to your own.

```sh
rn-tizen package
rn-tizen package --profile Store
rn-tizen package --dev --device 192.168.1.50
```

### `rn-tizen run`

Packages a development build of the app, installs it on a TV, launches it,
then prints its logs until Ctrl+C. Takes every `package` option but `--dev`,
plus:

| Option | |
|---|---|
| `--release` | Build for the store instead: the app streams no logs, so nothing prints |
| `--level <level>` | The quietest log level to show: `debug`, `info`, `warn` or `error` (default `info`) |
| `--no-logs` | Exit once the app is launched |

Without `--device`, it uses the only TV reached before that answers. `Install`
pushes the package to the TV and installs it, replacing any earlier install;
`Launch` starts it. The log listener starts before the build, so a port already in
use fails before anything compiles, and nothing the app logs at startup is lost.

```sh
rn-tizen run --device 192.168.1.50
rn-tizen run --level debug
```

### `rn-tizen logs`

Prints what a development build streams, from any number of launches, until
Ctrl+C. Use it to follow an app launched from the TV's own menu.

| Option | |
|---|---|
| `--port <port>` | The port the app was packaged with (default `8099`) |
| `--level <level>` | The quietest level to show (default `info`) |

### `rn-tizen perf`

Prints, once a second, where a development build's time goes: the frames the
main loop drew and dropped, Evas render times, how busy the main loop and the
JavaScript thread were, Fabric's commits and mounts, the Hermes heap and the
CPU. At Ctrl+C, or after `--duration`, it prints the run's average, 95th
percentile and worst of each. It listens on the log port, as `logs` does, and
asks every app that connects for samples: an app measures nothing until asked,
and stops when `perf` exits. A release build has none of it.

```sh
rn-tizen run --device 192.168.1.50 --no-logs
rn-tizen perf --device 192.168.1.50
rn-tizen perf --port 8102 --duration 20 --out boot.jsonl
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

Each line reads:

```
09:00:05  fps  58/60  drop   2  render   3.1/   9.4 ms  main  21%  js  35%  wait   4%  lag    2.0 ms  commit   3x  12.1/  30.2 ms  mount   4.0/   9.0 ms  heap  12.5/   32 MB  cpu  45%
```

A second in which nothing drew, committed or ran prints `idle`. The fields,
as `--out` saves them (times in milliseconds):

| Field | Meaning |
|---|---|
| `t`, `window` | When the sample ended, since the app started, and how long it covers (about 1000) |
| `frames` | Canvas renders that finished (`EVAS_CALLBACK_RENDER_POST`) |
| `renderAvg`, `renderMax` | Time from `EVAS_CALLBACK_RENDER_PRE` to `RENDER_POST`, per frame |
| `ticks`, `hz` | Ecore animator ticks while the main loop was free, and the display rate their median interval gives |
| `dropped` | Display frames the main loop missed while the canvas had something to draw |
| `mainBusy` | Time the main loop spent awake: input, mounting, animation, and rendering when the engine renders synchronously |
| `jsBusy` | Time the JavaScript thread ran tasks |
| `jsWait` | Time a worker, the JavaScript thread measuring text above all, waited for the main loop |
| `jsLag` | How long the last task queued on the JavaScript thread waited before it ran |
| `commits`, `commitAvg`, `commitMax`, `layoutMax` | Fabric commits mounted in the window, and the commit (with its layout) time on the JavaScript thread |
| `mutations` | View mutations mounted |
| `mountAvg`, `mountMax` | Diff and mount time on the main loop, per commit |
| `mountLatencyMax` | The longest wait between a commit ending and the main loop mounting it |
| `heapUsedMb`, `heapSizeMb`, `gcs` | Hermes's `getHeapInfo`: allocated bytes, heap size, collections so far |
| `cpu` | The process's CPU time, in percent of one core |

### `rn-tizen simulator`

Runs the app on a virtual TV on this Mac (Apple silicon, with
[Lima](https://lima-vm.io)), headless or with a live screen in the browser, and
drives it. The first run creates the VM and compiles React Native for it;
later runs rebuild only what changed.

```sh
rn-tizen simulator run --gui           # build, bundle, start, open the live screen
rn-tizen simulator key Down Enter      # press remote keys
rn-tizen simulator screenshot --out shot.png
rn-tizen simulator crash               # backtraces of the last native crash
rn-tizen simulator perf --duration 20  # frame times, thread load and heap
rn-tizen simulator run --instance ui-a  # a second app on its own screen
rn-tizen simulator run --entry src/screens/svg/main.ts  # one screen
rn-tizen simulator tile myapp://item/42  # open the app as a Smart Hub tile does
rn-tizen simulator voice '{"type":"selectTitle","title":"Dune"}'  # a voice command, as the TV shell sends it
rn-tizen simulator clean               # delete the runtime builds no instance runs
```

The VM holds any number of **instances**, each with its own screen, app, live
screen and log port, so several people or agents use it at once. Every action
takes `--instance <name>`, else `$RN_TIZEN_SIMULATOR_INSTANCE`, else `default`;
`status` lists every instance with its display and ports.

The runtime compiles the app's native libraries in, so the VM keeps one
runtime build per set of them: apps with the same libraries share one, and
each instance runs the build it was last staged on. The VM keeps the four most
recently used; `rn-tizen simulator clean` deletes every build no instance runs.

Every action, and how the simulator works, is in
[`simulator/README.md`](../simulator/README.md).

## Logs

Retail TVs do not let a developer read the system log, so a development build
streams JSON lines (`{"level":"info","message":"..."}`) over TCP to the address
in its `react-native-tizen.json`. `rn-tizen` prints each one as
`HH:MM:SS level message`, the level colored, a stack trace indented under its
message. The TV must reach this computer on that port: allow incoming
connections if a firewall asks.

A log line is untrusted input. `rn-tizen` drops its control characters, all
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
