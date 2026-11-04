# Benchmarks

What react-native-tizen costs on a real TV: how long a cold start takes to
draw, the frame rate of a long list and of a native-driver animation, the
memory the app holds, and the size of its package. Every number here was
measured on the TV below with the commands below. None is estimated.

## Device and build

| | |
|---|---|
| TV | Samsung GQ75LS03DAUXZG (The Frame, 2024) |
| Tizen | 9.0, armv7 (`cpu_arch:armv7` in sdb's capabilities) |
| Package floor | 9.0, the default at f6a4af1: the runtime linked against the 9.0 rootstrap, the shell built for `tizen90` |
| UI plane | 1920x1080. The animator ticks at 50 Hz (`hz` in `rn-tizen perf`) |
| Network | the TV and the Mac on the same home LAN |
| Build machine | Apple M2 Max, macOS 27.2 |
| react-native-tizen | `main` at f6a4af1, plus the bench entry from this page's pull request |
| App | `example/` with the entry `src/screens/bench/main.ts`: Expo SDK 58, react-native-tvos 0.88.0-0, React 19.3, locales `en` and `fr` |
| Date | 2026-11-04 |

## Results

| What | Median | Runs |
|---|---|---|
| Cold start, launch command to first frame, release build | 1,196 ms (1,120 to 2,603) | 17 launches |
| FlatList of 1,000 rows, focus one row every 200 ms | 54.1 frames/s, 22 dropped frames in 114 s | 3 runs |
| The same, one row every 50 ms | 52.9 frames/s, 135 dropped frames in 115 s, JavaScript thread 100% busy | 3 runs |
| 12 views on the native animated driver | 49.9 frames/s, 0 dropped frames in 84 s | 3 runs |
| Resident memory, release build, list still | 63.7 MB | 3 runs |
| Resident memory, release build, scrolling at 200 ms | 94.4 MB, peak 103.9 MB | 3 runs |
| `.tpk` size, release build | 8.0 MB (7,997,694 bytes) | 1 build |
| The same screen as a Tizen web app (react-native-web), cold start | 2,075 ms (1,390 to 2,892) | 17 launches |
| The web app's `.wgt` | 0.16 MB (163,032 bytes) | 1 build |

Frame counts come from a development build, because only a development build
answers `rn-tizen perf`. It runs the development runtime, which carries Hermes'
debugger and the dev server tooling. A release build was not measured for
frames.

## The app

`example/src/screens/bench/` is a screen like a TV catalog's:

- **FlatList.** 1,000 rows of 96 points, each a colored tile and two lines of
  text, with `getItemLayout`. Focus moves down one row on a timer, and an
  animated `scrollToIndex` keeps the focused row in the middle. The list has a
  fixed height of 780 points.
- **Native animated driver.** 12 views loop `translateX` over 1,400 points and
  `opacity`, with `useNativeDriver: true`, a 3 s cycle each, started 120 ms
  apart.
- **Reports.** The screen posts to `EXPO_PUBLIC_BENCH_REPORT_URL`: the first
  animation frame after its first mount, and once a second the process's
  `VmRSS` and `VmHWM`, read from `/proc/self/status` through expo-file-system.
- **Timeline.** With `EXPO_PUBLIC_BENCH_TIMELINE=1`, the screen ignores the
  remote and runs a fixed sequence from mount: 20 s still, 40 s with focus
  moving every 200 ms, 40 s every 50 ms, then 30 s of the animation. It posts
  each phase as it starts. Remote presses nobody sent reached the app during
  earlier runs, so the remote cannot change a measurement.

`example/scripts/bench-server.ts` is the machine side: it receives the reports
and, for cold starts, launches the app.

## Method

### Cold start

The release package was installed once. `bench-server.ts cold-start` then
launched it 17 times over sdb: 12 launches 8 s apart, then 5 launches 60 s
apart. For each launch it opens an sdb connection, reads the TV's capabilities,
starts a clock, sends `0 execute <app id>`, and stops the clock when the app's
first-frame report arrives over HTTP. It answers that report with `exit`, and
the app calls `BackHandler.exitApp()`. Every launch got a new process id from
the TV, so each one started a new process.

The time covers the TV's launcher, the .NET shell, loading
`libreact-native-tizen.so`, Hermes loading the bytecode, the first React
render, Fabric's first mount, the first frame, and one HTTP request on the LAN.
The TV answered `0 execute` after a median of 779 ms, and the first frame
arrived a median of 438 ms after that answer. The first launch after the
install took 2,603 ms. The others took 1,120 to 1,372 ms. Launches 8 s apart
and 60 s apart gave medians of 1,206 ms and 1,196 ms.

### Frame rate

The development package was installed and launched 3 times, each time with
`rn-tizen perf --duration 135 --out <file>` running from the launch to the end
of the timeline. `perf` reports a sample about every 1.24 s. Each sample was
assigned to a phase by the time it printed, and the first 2 s of each phase
were left out.

- **Frames/s** is `frames * 1000 / window`: canvas renders that finished
  (`EVAS_CALLBACK_RENDER_POST`) per second. The median is over every sample of
  the phase across the 3 runs.
- **Dropped** is the sum of `dropped`: display ticks the main loop missed while
  the canvas had something to draw.
- The animator ticked at 50 Hz in every sample. While the list scrolled, the
  canvas rendered more often than the animator ticked.

| Phase | Frames/s (median, 5th percentile) | Dropped | JavaScript thread busy | Main loop busy | Render, 95th percentile of the slowest per sample | Hermes heap |
|---|---|---|---|---|---|---|
| Still | 0 | 0 in 54 s | 0.2% | 0.4% | | 5.1 MB |
| Focus every 200 ms | 54.1, 51.7 | 22 in 114 s | 58% | 40% | 9.3 ms | 13.6 MB |
| Focus every 50 ms | 52.9, 50.7 | 135 in 115 s | 100% | 45% | 9.1 ms | 13.0 MB |
| Animation | 49.9, 49.6 | 0 in 84 s | 0.2% | 10% | 5.3 ms | 13.1 MB |

At one row every 50 ms the JavaScript thread is busy the whole time, and about
one frame a second drops. During the animation the JavaScript thread is idle:
the native driver runs it on the main loop.

### The same screen as a Tizen web app

The bench's components were also built for the web with react-native-web,
packaged as a Tizen web app, installed on the same TV and launched with the
same harness, 12 times 8 s apart and 5 times 60 s apart.

- The web app is a blank Expo SDK 58 app (`create-expo-app --template
  blank-typescript`, then `npx expo install react-dom react-native-web
  @expo/metro-runtime`) holding copies of `list-bench.tsx`,
  `animation-bench.tsx`, `bench-screens.tsx` and `bench-timeline.ts`. Two
  files differ. `useTVEventHandler` comes from a `keydown` listener, and the
  report hook exits with `tizen.application.getCurrentApplication().exit()`
  and reads no memory. The list has the same fixed height.
- `EXPO_PUBLIC_BENCH_REPORT_URL=... npx expo export -p web` wrote 517 KB of
  minified JavaScript. Its script path was made relative, and a `config.xml`
  with the `internet` privilege and `required_version="3.0"` was added. The
  package was signed with the same profile.

| Cold start, launch command to first frame | Native `.tpk` (release) | Web `.wgt` |
|---|---|---|
| 12 launches 8 s apart | 1,206 ms | 2,078 ms |
| 5 launches 60 s apart | 1,196 ms | 1,654 ms |
| All 17 | 1,196 ms | 2,075 ms |
| The TV answered `0 execute` after | 779 ms | 1,308 ms |
| Package | 8.0 MB | 0.16 MB |

Both apps got a new process on every launch. The web series started 23 minutes
after the native one ended. The web app's `.wgt` is small because the TV already
carries its browser engine. The native package carries Hermes, React Native and
ICU.

Frame rate and memory were not compared. A web app cannot read the canvas
render counter `rn-tizen perf` reads, and `requestAnimationFrame` callbacks
count the page's free main thread, not frames drawn. Its process's resident
memory is out of the page's reach.

### Memory

The release package was installed and launched 3 times and ran the timeline
each time. Each phase's figure is the median of the app's `VmRSS` reports in
that phase, and the table gives the median of the 3 runs.

| Phase | Resident memory (release) | Resident memory (development) |
|---|---|---|
| Still | 63.7 MB | 64.3 MB |
| Focus every 200 ms | 94.4 MB | 92.1 MB |
| Focus every 50 ms | 90.4 MB | 91.2 MB |
| Animation, after the list unmounted | 87.8 MB | 88.5 MB |
| Peak (`VmHWM`) | 103.9 MB | 104.2 MB |

`VmRSS` counts every page the process holds: the .NET runtime of the shell,
`libreact-native-tizen.so`, EFL, the GPU driver's mappings, Hermes and the
app's views.

### Package size

`rn-tizen package` with the bench entry, release build:

| File | Size | In the `.tpk` (deflated) |
|---|---|---|
| `lib/libreact-native-tizen.so` | 17.2 MB | 6.0 MB |
| `res/icudt78l.dat` (ICU data for `en` and `fr`) | 2.6 MB | 1.0 MB |
| `res/index.tizen.hbc` (Hermes bytecode) | 2.0 MB | 0.9 MB |
| The rest: the .NET shell, the preview service, signatures, notices, icon | | 0.05 MB |
| **The `.tpk`** | | **8.0 MB** |

The runtime ships with its symbol table. Stripped with `llvm-strip`, the same
library is 11.8 MB. The bytecode holds FormatJS's `Intl` data for the two
locales the example lists.

## Reproduce

From `example/`, with a TV in Developer mode and a signing profile. The runs
above used a 9.0 floor, so the commands pass `--platform 9.0`; since 6.0
became the default floor, every package's shell lands in `tizen80/`:

```sh
export EXPO_PUBLIC_BENCH_REPORT_URL=http://<this machine>:47200 EXPO_PUBLIC_BENCH_TIMELINE=1

# release: cold start and memory
rn-tizen package --platform 9.0 --entry src/screens/bench/main.ts
cp tizen/bin/Release/tizen80/org.rntizen.example-1.0.0.tpk /tmp/bench-release.tpk
rn-tizen install /tmp/bench-release.tpk --device <tv ip>
BENCH_SETTLE_MS=8000 bun scripts/bench-server.ts cold-start <tv ip> /tmp/bench-release.tpk 12
bun scripts/bench-server.ts listen > release.jsonl &
rn-tizen install /tmp/bench-release.tpk --device <tv ip> --keep-signature --launch

# development: frames
rn-tizen package --dev --platform 9.0 --device <tv ip> --entry src/screens/bench/main.ts
rn-tizen install tizen/bin/Release/tizen80/org.rntizen.example-1.0.0.tpk --device <tv ip> --keep-signature --launch
rn-tizen perf --device <tv ip> --duration 135 --out perf.jsonl
```

## Raw results

### Cold start, per launch

| Build | Spacing | Launch | `0 execute` answered (ms) | First frame (ms) |
|---|---|---|---|---|
| native release | 8 s | 1 | 1716 | 2603 |
| native release | 8 s | 2 | 875 | 1227 |
| native release | 8 s | 3 | 662 | 1132 |
| native release | 8 s | 4 | 775 | 1298 |
| native release | 8 s | 5 | 718 | 1120 |
| native release | 8 s | 6 | 788 | 1222 |
| native release | 8 s | 7 | 762 | 1223 |
| native release | 8 s | 8 | 866 | 1190 |
| native release | 8 s | 9 | 779 | 1183 |
| native release | 8 s | 10 | 884 | 1322 |
| native release | 8 s | 11 | 818 | 1148 |
| native release | 8 s | 12 | 747 | 1163 |
| native release | 60 s | 1 | 804 | 1349 |
| native release | 60 s | 2 | 746 | 1196 |
| native release | 60 s | 3 | 740 | 1191 |
| native release | 60 s | 4 | 1021 | 1372 |
| native release | 60 s | 5 | 696 | 1155 |
| web | 8 s | 1 | 519 | 1390 |
| web | 8 s | 2 | 2077 | 2892 |
| web | 8 s | 3 | 1329 | 2187 |
| web | 8 s | 4 | 1293 | 1977 |
| web | 8 s | 5 | 1308 | 2076 |
| web | 8 s | 6 | 1280 | 2092 |
| web | 8 s | 7 | 1423 | 2080 |
| web | 8 s | 8 | 1360 | 2070 |
| web | 8 s | 9 | 1333 | 2171 |
| web | 8 s | 10 | 1323 | 2010 |
| web | 8 s | 11 | 1272 | 2075 |
| web | 8 s | 12 | 1308 | 2082 |
| web | 60 s | 1 | 1361 | 2135 |
| web | 60 s | 2 | 513 | 1718 |
| web | 60 s | 3 | 508 | 1654 |
| web | 60 s | 4 | 464 | 1552 |
| web | 60 s | 5 | 302 | 1419 |

### Memory, release build, per run (median VmRSS of each phase, MB)

| Run | Still | Focus every 200 ms | Every 50 ms | Animation | Peak (VmHWM) |
|---|---|---|---|---|---|
| 1 | 63.7 | 94.4 | 90.5 | 88.0 | 103.9 |
| 2 | 63.9 | 91.1 | 90.3 | 87.3 | 106.8 |
| 3 | 63.7 | 95.3 | 90.4 | 87.8 | 103.7 |

### Frames, development build, per run

| Run | Phase | Seconds sampled | Frames/s (median) | Dropped | JS busy (median) | Main loop busy (median) | Hermes heap (median, MB) |
|---|---|---|---|---|---|---|---|
| 1 | Still | 17 | 0.0 | 0 | 0% | 0% | 5.1 |
| 1 | Every 200 ms | 38 | 54.1 | 6 | 59% | 39% | 13.9 |
| 1 | Every 50 ms | 38 | 52.9 | 34 | 100% | 45% | 13.5 |
| 1 | Animation | 28 | 49.8 | 0 | 0% | 10% | 13.5 |
| 2 | Still | 19 | 0.0 | 0 | 0% | 0% | 5.1 |
| 2 | Every 200 ms | 38 | 54.3 | 6 | 58% | 40% | 14.3 |
| 2 | Every 50 ms | 38 | 52.7 | 46 | 100% | 46% | 12.9 |
| 2 | Animation | 27 | 49.9 | 0 | 0% | 11% | 13.1 |
| 3 | Still | 18 | 0.0 | 0 | 0% | 0% | 5.1 |
| 3 | Every 200 ms | 38 | 53.3 | 10 | 56% | 41% | 13.4 |
| 3 | Every 50 ms | 39 | 53.3 | 55 | 100% | 45% | 13.0 |
| 3 | Animation | 28 | 49.9 | 0 | 0% | 10% | 12.1 |
