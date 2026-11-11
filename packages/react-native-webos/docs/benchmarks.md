# Benchmarks

What react-native-webos costs, as far as it can be measured without a TV.
Every number here comes from the simulator, on the machine below, with the
commands below. None is estimated, and none says how fast an LG TV is.

## What these numbers are good for

The simulator draws with Mesa's llvmpipe, a software renderer on the Mac's
CPU, in a Linux VM. A TV draws on its own GPU, with its own processor. So:

- A number that compares two builds on the same machine means something: it is
  how the changes under [What changed, and by how much](#what-changed-and-by-how-much)
  were judged.
- A frame rate, a start time or a memory figure says nothing about a TV. The
  process memory in particular is mostly the software renderer and LLVM, which
  a TV does not load.
- Package and executable sizes are the TV's own: they are the files a TV gets.

The same screen and the same commands run on a TV, where `rn-webos perf` reads
the same counters. Those numbers go here, with the TV's model, year and webOS
version, once a TV has run it.

## Machine and build

| | |
|---|---|
| Machine | Apple M2 Max, macOS 27.2 |
| Simulator | Lima VM `rn-webos-sim`: Ubuntu 26.04 aarch64, 8 CPUs, 12 GiB |
| Renderer | OpenGL ES 3.2, Mesa 26.0.8 llvmpipe (LLVM 21.1.8), on Xvfb, 1920x1080 |
| Display pacing | none: Xvfb reports no refresh rate, so the host paces frames at 60 Hz itself |
| react-native-webos | `main` on 2026-11-11, development runtime (`RelWithDebInfo`) |
| App | `example/` with the entry `src/screens/bench/main.ts`: Expo SDK 58, react-native-tvos 0.88.0-0, React 19.3, react-native-svg compiled in, locales `en` and `fr` |

## Results

20 s of `rn-webos simulator perf` in each phase of the bench screen:

| Phase | Frames/s | Dropped | Render, average and 95th percentile | Main loop busy | JavaScript thread busy |
|---|---|---|---|---|---|
| Still | 0.1 | 0 | 0.6 ms | 0.3% | 0.1% |
| FlatList of 1,000 rows, focus one row every 200 ms | 60.0 | 0 of 1,206 | 0.3 ms, 1.5 ms | 2.9% | 5.2% |
| The same, one row every 50 ms | 60.0 | 0 of 1,205 | 0.3 ms, 1.5 ms | 4.5% | 17.5% |
| 12 views on the native animated driver | 60.0 | 0 of 1,205 | 0.2 ms, 1.3 ms | 2.4% | 0.0% |

"Render" is the time the main loop spends walking the views and handing Skia
their drawing. Rasterizing happens on llvmpipe's own threads and is not in it.
A still screen draws nothing: the 0.1 frames a second are the screen's memory
label, the two times its text changed.

| What a TV gets | |
|---|---|
| The release executable, `react-native-webos` | 20.2 MB (20,224,556 bytes), 32-bit ARM, Thumb-2 |
| The release package of the example | 10.9 MB (10,947,530 bytes), 25.2 MB installed |

| Start, in the simulator | |
|---|---|
| Launch to the bench screen's first frame, development build | 257 to 277 ms over 28 launches, read from the app's log every 5 ms |

## What changed, and by how much

Each row compares two builds that differ by one change, on the bench screen at
one row every 50 ms unless it says otherwise.

| Change | Before | After |
|---|---|---|
| A paragraph is laid out once, on the layout thread, and its view plays the recording (React Native's prepared text layout) | render 4.3 ms at the 95th percentile, 4.5 ms at worst; 5.7 ms mounting paragraphs in 20 s | 1.4 ms, 1.7 ms; 3.6 ms |
| Frames are paced from when each might have started, not from a timer's late wake | 50.8 frames/s, the loop ticking at 51.9 Hz | 60.0 frames/s, 60.1 Hz |
| A hidden app frees Skia's GPU caches, collects JavaScript garbage and returns freed heap | 173.6 MB of process memory after a scroll | 167.1 MB while hidden; 4.1 MB of it GPU caches by Skia's count |
| Skia keeps compiled shaders on disk between runs | 257 to 277 ms to the first frame without the cache | 257 to 272 ms with it: no difference here, where a shader compiles in no time. 35 programs, 356 KB on disk after visiting every example screen |

The screenshots of the example's rendering screens were compared pixel for
pixel across the first and the fourth change. The four text screens are
identical with the first. With the fourth, ten screens are identical, the
transform screen has one pixel one level off, and the three screens that
scroll by themselves were caught at another position.

The shader cache, the cache limit and the missing stencil buffers follow what
Flutter does on the same renderer
([skia-rendering.md](reference/skia-rendering.md), section 7). What they are
worth on a TV's GPU is not known.

## The app

`example/src/screens/bench/` is a screen like a TV catalog's:

- **FlatList.** 1,000 rows of 96 points, each a colored tile and two lines of
  text, with `getItemLayout`. Focus moves down one row on a timer, and an
  animated `scrollToIndex` keeps the focused row in the middle. Enter steps
  through still, 200 ms and 50 ms.
- **Native animated driver.** 12 views loop `translateX` over 1,400 points and
  `opacity`, with `useNativeDriver: true`, a 3 s cycle each, started 120 ms
  apart. Right shows it.
- **Reports.** The screen logs the first animation frame after its first
  mount, and posts it to `EXPO_PUBLIC_BENCH_REPORT_URL` when that is set, with
  the process's `VmRSS` and `VmHWM` once a second.
- **Timeline.** With `EXPO_PUBLIC_BENCH_TIMELINE=1`, the screen ignores the
  remote and runs a fixed sequence from mount: 20 s still, 40 s with focus
  moving every 200 ms, 40 s every 50 ms, then 30 s of the animation.

## Reproducing

```sh
cd example
rn-webos simulator run --instance bench --entry src/screens/bench/main.ts
# once the app is up, stop that command with Ctrl+C: the app keeps running
rn-webos simulator perf --instance bench --duration 20       # still
rn-webos simulator key Enter --instance bench                 # one row every 200 ms
rn-webos simulator perf --instance bench --duration 20
rn-webos simulator key Enter --instance bench                 # one row every 50 ms
rn-webos simulator perf --instance bench --duration 20
rn-webos simulator key Right --instance bench                 # the animation
rn-webos simulator perf --instance bench --duration 20
```

`perf --out <file.jsonl>` keeps every sample. The fields are in
[cli.md](cli.md#rn-webos-perf).

On a TV, the same with `rn-webos run --no-logs --entry ...` and
`rn-webos perf --device <ip>`. For cold starts, package the bench entry with
`EXPO_PUBLIC_BENCH_REPORT_URL=http://<this computer>:47200`, install it, and
run `bun scripts/bench-server.ts cold-start <tv> <app id>`, which launches the
app and times each launch to its first-frame report. That script has not run
against a TV.

The sizes come from `rn-webos package` in `example/`, and the TV executable
was checked with `bun scripts/check-tv-binary.ts example/webos/app/react-native-webos`.
