# Contributing

Bug reports, fixes and new platform coverage are welcome. Open an issue before a
large change so we can agree on the approach first.

## What you need

The TypeScript side (the `rn-webos` CLI, the Metro integration, the tests) needs
only Bun. Building and running an app needs the rest. macOS on Apple silicon is
the only host verified so far.

- [Bun](https://bun.sh) 1.4 or newer. CI runs the version the
  root `package.json` names.
- Homebrew's LLVM and lld, which cross-compile the runtime, plus the build tools:

  ```sh
  brew install llvm lld cmake ninja ccache
  ```

  CMake must be 3.24 or newer. ccache is optional. Set `WEBOS_LLVM_DIR` to use
  a clang and lld that are not Homebrew's. The build also needs git, Python 3
  and a compiler for its own tools, which Xcode's command line tools bring, and
  GN, which it downloads when none is installed.
- The SDK TV builds compile against, openlgtv's: `rn-webos` downloads it into
  the cache on the first build, or ahead of time with `setup`. Nothing of LG's
  is installed: `rn-webos` reaches the TV and writes packages itself, and a
  webOS package is not signed.
- Node.js, which runs Expo's CLI when the app is bundled. `npx expo start` in
  the example needs 22.18 or newer: its Metro config imports `withTV` from
  `src/`, so Node must strip TypeScript types on its own.
- For the simulator: [Lima](https://lima-vm.io) 2.0 or newer (`brew install
  lima`) and about 8 GB of free disk.

`rn-webos doctor` checks all of this and prints the fix for anything missing.

## Set up

```sh
git clone https://github.com/kromatv/react-native-tv.git
cd react-native-tv
bun install
cd packages/react-native-webos
```

The package is `packages/react-native-webos`, and every command below runs from
there. `example/` is an Expo app in the same Bun
workspace. `bun install` builds `dist/` (the `prepare` script), which the
example's `app.json` loads the config plugin from through `../app.plugin.js`;
its `metro.config.mjs` imports `../src/metro/with-webos.ts` directly. Run the CLI
from source with `bun src/cli/main.ts <command>`, and point it at the example
with `--project example`:

```sh
bun src/cli/main.ts doctor --project example
bun src/cli/main.ts setup
```

`setup` downloads the SDK. `package`, `run` and `build-runtime` do this on
their own, so run it by hand only to prepare a machine ahead of time.

## The loop

1. Change the code.
2. Run the checks CI runs:

   ```sh
   bun run typecheck
   bun run test
   bun run check        # oxlint and oxfmt; bun run check:fix rewrites
   bun run build        # dist/, the compiled JavaScript npm ships
   find simulator -name '*.sh' -print0 | xargs -0 shellcheck -x
   ```

   A change in `runtime/` also runs the host's C++ tests: doctest cases in
   `*.test.cpp` beside the code they test, built and run in the simulator's VM
   (CI runs them on aarch64 Linux, and reports their coverage to SonarCloud).
   The VM needs a runtime built once, by `simulator build` or `simulator run`.

   ```sh
   bun run test:runtime
   ```

3. Try it in the simulator, a virtual TV in a Lima VM that builds the same runtime
   for aarch64 Linux. The first run creates the VM and compiles React Native, which
   takes about 20 minutes; later runs rebuild only what changed.

   ```sh
   bun src/cli/main.ts simulator run --project example --gui
   bun src/cli/main.ts simulator key Down Enter
   bun src/cli/main.ts simulator screenshot --out /tmp/screen.png
   bun src/cli/main.ts simulator logs -n 50
   bun src/cli/main.ts simulator crash
   ```

   After a change in `runtime/`, rebuild it and restart the app:
   `simulator build`, then `simulator restart`. Pass `--instance <name>` to get a
   screen of your own when someone else uses the same VM.
   [`simulator/README.md`](simulator/README.md) has every action and the
   simulator's limits.

4. Check it on a TV. On the TV, install the Developer Mode app from LG's
   store, sign in with an LG developer account, turn Dev Mode Status on, and
   after the restart turn Key Server on. Then:

   ```sh
   bun src/cli/main.ts connect 192.168.1.50
   bun src/cli/main.ts run --project example --device 192.168.1.50
   ```

   `connect` asks for the passphrase the Developer Mode app shows. `run`
   packages the app, installs it, launches it and prints its logs until
   Ctrl+C. A Developer Mode login cannot read the TV's system log, so a
   development build streams its logs to the CLI over TCP: allow incoming
   connections if the firewall asks. A TV build that never reached a TV is
   checked against firmware symbol data instead:
   `bun scripts/check-tv-binary.ts example/webos/app/react-native-webos`.

[`docs/cli.md`](docs/cli.md) documents every command and option.

## Platform facts

Code that depends on how webOS, Skia, SDL, React Native or Expo behave follows
[`docs/reference/`](docs/reference/README.md). If the fact you need is not there,
find it in the official documentation, the SDK headers or a project that ships
on the platform, and add it to the reference, cited, in the same pull request.
Something you saw on a TV goes in with the model, the year and the webOS version.
[`docs/decisions.md`](docs/decisions.md) records the choices the package is built
on; a change to one of them changes that file too.

## Pull requests and releases

The repository's [`CONTRIBUTING.md`](../../CONTRIBUTING.md) has the rules for a
pull request and how a release goes out. In the pull request, say how you
tested: the simulator, a TV (model and webOS version), or tests only.
