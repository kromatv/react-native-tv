# webOS TV simulator

A virtual LG TV on an Apple silicon Mac. `rn-webos simulator` builds the same
runtime a TV runs (the host in `runtime/host`, Hermes, Fabric, Expo Modules,
drawn with Skia in a window SDL opens) for aarch64 Linux, runs the app on a
1920x1080 screen in a local VM, and lets you watch it, drive it with a remote
and its pointer, read its logs and debug its crashes. It runs headless, for
scripts and agents, or with a live screen in the browser.

LG's own webOS TV Simulator runs web apps only, and Apple silicon cannot run
the 32-bit ARM code a TV runs, so the simulator is a native aarch64 Linux VM
under Apple's own hypervisor.

## Quick start

```sh
brew install lima
cd my-app
rn-webos simulator run          # headless: build, bundle, start, print the logs
rn-webos simulator run --gui    # the same, and open the live screen and remote
```

The first `run` creates the VM (Ubuntu 26.04, a few minutes) and compiles Skia
and React Native for it (about 20 minutes). After that, `run` rebuilds only what changed
and takes seconds. Ctrl+C stops printing logs; the app keeps running for the
commands below.

## Headless or with a live screen

The app always draws into a virtual screen in the VM. Headless is the default:
`screenshot`, `key` and `logs` work on it with nothing else running.

`rn-webos simulator view` (or `run --gui`) opens the live screen in the
browser, at `http://127.0.0.1:40000/` for the default instance (`status` prints
every instance's address): the TV picture, updated as it changes,
next to a remote with the D-pad, Back, Exit, media and colour keys. While the
page has the focus, the arrows, Enter, Esc or Backspace (Back) and Space (Play)
work as the remote. The page stays connected across `restart`; after `stop`,
it reconnects once `view` runs again. Headless commands keep working while it
is open.

## Commands

Run them in the app's directory, or pass `--project <dir>`. `--verbose` shows
everything the tools inside print. Every action takes `--instance <name>` (see
[Several instances](#several-instances)); without it, the instance is
`$RN_WEBOS_SIMULATOR_INSTANCE`, else `default`.

| Command | What it does |
| --- | --- |
| `run` | Creates or starts the VM, builds the runtime, bundles the app with Metro and Hermes, starts it and prints its logs. `--bundle <file.hbc>` starts a ready bundle instead; `--no-logs` returns once the app is up; `--level` works as for `rn-webos run`. The logs come in on the instance's own port; `--log-port` picks another. |
| `view` | Opens the live screen and remote of the running app. |
| `screenshot [--out <file.png>]` | Saves the screen as a PNG and prints the path. Default `~/.cache/react-native-webos/simulator/<instance>/screenshot.png`. |
| `key <key>...` | Presses remote keys in order: `key Down Down Enter`. `<key>:<milliseconds>` holds one: `key Enter:800`. |
| `text <text>` | Types into the focused text field, as the TV's on-screen keyboard commits text. |
| `pointer <x> <y>`, `click [<x> <y>]`, `wheel <notches>` | Moves the remote's pointer to a screen pixel, presses its button there, or turns its wheel: `wheel -3` scrolls down three notches. |
| `pause`, `resume` | Sends the app to the background and back: `AppState` changes as on a TV. |
| `open-url <url>` | Delivers a deep link, as the TV does when another app opens this one. |
| `relaunch [--params <json>]` | Launches the running app again with new parameters, as the TV's app manager does: `relaunch --params '{"contentTarget":"myapp://item/42"}'`. `run` and `restart` take `--params` too, for the first launch. |
| `restart` | Starts the staged app again, without rebuilding. |
| `build` | Rebuilds the app's native runtime only, after a change in `runtime/`; `restart` then runs it. Apps with the same native libraries share it (see [Runtime builds](#runtime-builds)). |
| `logs [-f] [-n <lines>]` | Prints the app's stdout and stderr, and follows them with `-f`. |
| `crash` | Prints the backtrace of every thread at the instance's last native crash, with file and line. |
| `perf [--duration <s>] [--out <file>]` | Prints the app's frame times, thread load and heap every second, then a summary, as [`rn-webos perf`](../docs/cli.md#rn-webos-perf) does, on the instance's log port. It needs an app `run` started without `--no-logs` (that one is a release build), and `run` no longer printing its logs. |
| `status` | Shows the VM and its runtime builds, then every instance: its app, its runtime, its screen, its live screen and its log port. |
| `clean` | Deletes every runtime build no instance runs. |
| `stop` | Stops the instance's app, screen and live screen. The VM and every other instance keep running. |
| `delete` | Deletes the VM, everything built in it and every instance. |

### Remote keys

`key` hands the app the key event the TV's remote sends, through the app's
control channel rather than the X server, so a key takes the same path as on a
TV and can be held for an exact time (see `runtime/host/src/input`):

| Key | What the app receives |
| --- | --- |
| `Left` `Right` `Up` `Down` | the arrow keys |
| `Enter` (or `Ok`, `Select`) | Return |
| `Back` | the remote's Back key |
| `Play` `Pause` `PlayPause` `Stop` `Record` | the media keys |
| `Rewind` `FastForward` `Previous` `Next` | the transport keys |
| `ChannelUp` `ChannelDown` `Guide` `Info` | the same |
| `Red` `Green` `Yellow` `Blue` | the colour keys |
| `0` to `9`, `Backspace`, `Escape`, `Tab` | the same |

The app logs a name it does not know and ignores it. Exit and Home never reach
an app on a TV, so the simulator has neither: `stop` ends the app. When Back
leaves the app at its root screen, as on a TV, `restart` starts it again.

The live screen takes the keyboard instead: the arrows, Enter, Esc or
Backspace (Back), Space (Play), F1 to F4 for the colour keys, F5 for Info, and
the mouse as the remote's pointer.

## Several instances

The VM runs any number of apps side by side, one per **instance**, so several
people or agents test at once without pressing keys on each other's app. Each
instance has its own virtual screen and window manager, its own app process and
staged bundle, its own live screen and its own log port on the Mac. `run` and
`restart` start one instance's app and leave every other running; `stop`
stops one instance; a crash takes down its own instance only, and `crash` reads
that instance's.

Name the instance once per shell, or on each command:

```sh
export RN_WEBOS_SIMULATOR_INSTANCE=checkout-focus
rn-webos simulator run --no-logs
rn-webos simulator key Down Enter
rn-webos simulator screenshot --out /tmp/checkout-focus.png

rn-webos simulator run --no-logs --instance settings-page   # the same, beside it
```

A name is 1 to 32 lowercase letters, digits, `-` or `_`. Its display and ports
come from the name alone: `default` takes slot 0, any other name slot
1 + FNV-1a(name) mod 999, and the slot `s` gives

| What | For slot `s` |
| --- | --- |
| Display in the VM | `:100+s` |
| Live screen on the Mac | `40000+s` |
| Log port on the Mac | `41000+s` |
| VNC in the VM (forwarded too) | `42000+s` |

so `default` is display `:100`, `http://127.0.0.1:40000/` and log port 41000.
Two names can land on the same slot; the second to start then fails, naming the
first, and needs another name.

The VM mounts, read-only and at the same path, the deepest directory holding
this package, the app's react-native and Expo packages and its native
libraries. It refuses one that is `/`, your home directory or a directory
above it: keep the app and its `node_modules` in one project directory.

What instances share: the VM, its mounts, and the runtime build of each set of
native libraries. A project no mount holds yet (another clone, another
worktree, an app with the package installed) gets its own mount beside the
others, which restarts the VM once and stops every instance in it; from then
on every project mounted runs without a restart. `limactl edit rn-webos-sim`
removes a mount you no longer need.

## Runtime builds

The runtime compiles the app's native libraries in (`react-native-svg`, an
Expo module, the app's own modules), so the VM keeps one build per set of
them, under `~/rn-webos-simulator/build/runtimes/<key>`. The key is the first
12 hex digits of a SHA-256 over this package's `runtime/` (real path), the
app's react-native, expo-modules-core and expo-modules-jsi (name, version,
real path) and each native library (name,
version, real path, the webOS implementation compiled for it). Two apps with
the same libraries share a build; an app with one more library gets its own and
keeps it warm, so running one app after the other rebuilds neither.

- `run` builds the app's runtime, then stages the app on it: the instance runs
  that build until it is staged again, and `restart` restarts it there. A build
  never moves another instance to other native code.
- One build of a directory runs at a time; another `run` or `build` of the same
  libraries prints that it is waiting and goes on when the first is done.
  Builds of other libraries run alongside. A build replaces the runtime
  under every instance staged on it, which each picks up at its next `run` or
  `restart`; an app already running keeps the library it loaded.
- Every build compiles through one ccache, hashed relative to its own
  directory (`CCACHE_BASEDIR`, `CCACHE_NOHASHDIR`), so a new set of libraries
  compiles only what the cache has not seen.
- `status` names the build each instance runs. `shared` is the one build every
  instance ran before builds had keys, at `build/runtime`: an instance staged
  then keeps it, running or not, until its next `run`.
- After a build, the VM keeps the 4 most recently used builds
  (`RN_WEBOS_SIMULATOR_RUNTIMES` in the VM changes the count) and deletes
  older ones, least recently used first, unless an instance is staged on one,
  one is building, or one was used in the last 30 minutes. `clean` deletes every
  build no instance is staged on and none is building, now. A build is about
  2 GB.

## Debugging

- **Logs.** `run` prints the app's logs as `rn-webos run` does for a TV, from
  the same log stream. `logs` prints everything the app wrote to stdout and
  stderr, including native lines the stream does not carry.
- **Native crashes.** The VM keeps a core of every crash (systemd-coredump), and
  `crash` symbolizes the instance's last one with gdb, every thread, down to the
  file and line in `runtime/host`. `status` says which app crashed.
- **An FHD TV.** An app started with `RN_WEBOS_SIMULATOR_SCREEN_SIZE=1280x720`
  gets a 1280x720 window, so the logical 1920x1080 screen draws at 2/3 in the
  screen's top-left corner. No command sets it yet: start the instance's
  launcher by hand from `limactl shell rn-webos-sim`, with the variables
  `guest/instance.sh` exports.
- **A debugger.** The runtime is built `RelWithDebInfo`; Skia is built optimized, without debug information.
  `limactl shell rn-webos-sim` opens a shell in the VM, where
  `gdb -p $(cat ~/rn-webos-simulator/instances/<instance>/app.pid)` attaches to
  the running app.

A loop that works for a developer and for an agent:

```sh
rn-webos simulator run --no-logs
rn-webos simulator key Down Enter
rn-webos simulator screenshot --out /tmp/screen.png   # then look at it
rn-webos simulator logs -n 50
# edit runtime/host, then:
rn-webos simulator build && rn-webos simulator restart
```

## How it works

| Piece | What it does |
| --- | --- |
| [`lima.yaml`](lima.yaml) | The VM: Ubuntu 26.04 LTS for aarch64 under Apple's Virtualization framework (Lima `vz`), 8 CPUs, 12 GiB. It mounts the app's project root read-only and the `react-native-webos` cache writable, both at their Mac paths, so a build sees the same absolute paths. |
| [`guest/`](guest) | The scripts the CLI runs in the VM. `provision.sh` installs the toolchain, GN, Mesa, SDL's X11 headers, Xvfb, matchbox, x11vnc, noVNC and systemd-coredump, and runs again only when it changes. |
| [`../runtime/cmake/Simulator.cmake`](../runtime/cmake/Simulator.cmake) | With `-DRN_WEBOS_SIMULATOR=ON`, the runtime builds natively on Linux: the OpenGL ES a TV's sysroot provides comes from Mesa, and SDL, built from the same source as for a TV, opens an X11 window instead of a webOS surface. `runtime/host` builds unchanged. |
| The app itself | The same `react-native-webos` executable as on a TV. It takes its directories from the environment (`RN_WEBOS_RESOURCE_DIR`, `RN_WEBOS_APP_DATA_DIR`), and a simulator build reads `pause`, `resume`, `open-url`, `exit`, remote keys and pointer moves from a control FIFO (`runtime/host/src/platform/SimulatorControl.h`). The TV's Luna bus does not exist here, so what depends on it answers as on a TV that refuses the call. |
| [`viewer/`](viewer) | The live screen: a page served by websockify in the VM, drawing the screen with noVNC from x11vnc, with the remote beside it. Lima forwards its port to the Mac. |
| [`../src/simulator/`](../src/simulator) | The CLI side: it drives the VM through `limactl`, stages the bundle as a TV package's `res` directory, and reads what the guest scripts report. |

Each instance's screen is Xvfb with matchbox, a kiosk window manager that
shows the app fullscreen and focused as a TV's compositor does. Builds stay on the VM's
disk under `~/rn-webos-simulator/build`, behind ccache, and each instance's
files under `~/rn-webos-simulator/instances/<instance>`, with the key of the
build it runs in `runtime`.

## Prerequisites and footprint

- An Apple silicon Mac and [Lima](https://lima-vm.io) 2.0 or later.
- About 8 GB of disk: the VM with its packages and one runtime build, and
  about 2 GB more per set of native libraries, four at most unless instances
  hold more. The VM disk is sparse and grows up to 60 GiB. `rn-webos simulator
  clean` frees the builds no instance runs, `rn-webos simulator delete` all
  of it.

## Limits

- **Close to a TV, not identical.** Skia, SkParagraph and the views are the
  same code, so layout and drawing match; the fonts (DejaVu and Noto here, LG's
  on a TV) and the OpenGL ES driver (Mesa's software renderer here, the TV's
  GPU there) differ, so font metrics and timings do.
- **No webOS services.** There is no Luna bus: no app manager, no settings
  service, no media pipeline. Video shows a placeholder where the picture goes.
- **No on-screen keyboard.** Text reaches a field through the control channel.
- **aarch64, not armv7l.** 64-bit pointers and ARMv8 atomics: a bug that only
  shows on a 32-bit TV does not reproduce here. The runtime links Ubuntu's
  shared libstdc++ and newer glibc, where a TV build carries its own libstdc++.
