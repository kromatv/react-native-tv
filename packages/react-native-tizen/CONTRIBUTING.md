# Contributing

Bug reports, fixes and new platform coverage are welcome. Open an issue before a
large change so we can agree on the approach first.

## What you need

The TypeScript side (the `rn-tizen` CLI, the Metro integration, the tests) needs
only Bun. Building and running an app needs the rest. macOS on Apple silicon is
the only host verified so far.

- [Bun](https://bun.sh) 1.4 or newer. CI runs the version in `.bun-version`.
- [Tizen Studio](https://developer.tizen.org/development/tizen-studio/download).
  From its Package Manager, install `IOT-Headed-6.0-NativeAppDevelopment-CLI`,
  the rootstrap for the default floor, Tizen 6.0, `NativeToolchain-Gcc-14.2`, the
  GCC runtime the runtime links against, and the Samsung Certificate Extension.
  Each other floor needs its own rootstrap package (`IOT-Headed-<v>-...` for 6.0
  to 7.0, `TIZEN-<v>-...` from 8.0); `bun src/cli/main.ts doctor --platform <v>`
  names it. Set `TIZEN_SDK_DIR` if it is not in
  `~/tizen-studio`. Its `sdb` and `tizen` tools are not used: `rn-tizen` speaks
  to the TV and signs packages itself.
- Homebrew's LLVM and lld, which cross-compile the runtime, plus the build tools:

  ```sh
  brew install llvm lld cmake ninja dotnet ccache
  ```

  CMake must be 3.24 or newer and the .NET SDK 6.0 or newer. ccache is optional.
  Set `TIZEN_LLVM_DIR` to use a clang and lld that are not Homebrew's.
- Node.js, which runs Expo's CLI when the app is bundled. `npx expo start` in
  the example needs 22.18 or newer: its Metro config imports `withTizen` from
  `src/`, so Node must strip TypeScript types on its own.
- A Samsung certificate profile, once: in Tizen Studio, Tools, Certificate
  Manager, create a Samsung profile with your TV's DUID and make it the active
  profile. Samsung issues its author and distributor certificates; `rn-tizen`
  reads them from that profile.
- For the simulator: [Lima](https://lima-vm.io) 2.0 or newer (`brew install
  lima`) and about 8 GB of free disk.

`rn-tizen doctor` checks all of this and prints the fix for anything missing.

## Set up

```sh
git clone https://github.com/kromatv/react-native-tizen.git
cd react-native-tizen
bun install
```

The repository root is the package. `example/` is an Expo app in the same Bun
workspace. `bun install` builds `dist/` (the `prepare` script), which the
example's `app.json` loads the config plugin from through `../app.plugin.js`;
its `metro.config.mjs` imports `../src/metro/with-tizen.ts` directly. Run the CLI
from source with `bun src/cli/main.ts <command>`, and point it at the example
with `--project example`:

```sh
bun src/cli/main.ts doctor --project example
bun src/cli/main.ts setup
```

`setup` compiles the glibc compat archive against the rootstrap and mirrors the
rootstrap into a sysroot lld can link against. `package`, `run` and
`build-runtime` do this on their own, so run it by hand only to prepare a machine
ahead of time.

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

4. Check it on a TV. On the TV, open Apps, type `12345` on the remote, turn
   Developer mode on with your computer's IP address, and restart the TV. Then:

   ```sh
   bun src/cli/main.ts devices --connect 192.168.1.50
   bun src/cli/main.ts run --project example --device 192.168.1.50
   ```

   `run` packages the app, installs it, launches it and prints its logs until
   Ctrl+C. Retail TVs block the system log, so a development build streams its
   logs to the CLI over TCP: allow incoming connections if the firewall asks.

[`docs/cli.md`](docs/cli.md) documents every command and option.

## Platform facts

Code that depends on how Tizen, EFL, React Native or Expo behave follows
[`docs/reference/`](docs/reference/README.md). If the fact you need is not there,
find it in the official documentation, the SDK headers or a project that ships
on the platform, and add it to the reference, cited, in the same pull request.
Something you saw on a TV goes in with the model, the year and the Tizen version.
[`docs/decisions.md`](docs/decisions.md) records the choices the package is built
on; a change to one of them changes that file too.

## Pull requests

- One change per pull request. It is squash-merged, so its title becomes the
  commit: conventional form, imperative, lowercase, such as
  `fix(input): map the Back key on Tizen 8`.
- Code, comments and docs are in English.
- Comments are rare. Document exported API whose contract the signature does not
  show, and explain a platform constraint the code cannot. Nothing else.
- A file over 300 lines is split at a natural seam.
- Data from outside the process (files, the network, a TV, a child process) is
  parsed with zod.
- Say how you tested: the simulator, a TV (model and Tizen version), or tests
  only.

## Releases

Maintainers tag `vX.Y.Z` on `main`, and the release workflow publishes that
version to npm with provenance.
