# Contributing

Bug reports, fixes and new platform coverage are welcome. Open an issue before a
large change so we can agree on the approach first.

## What you need

The TypeScript side (the `rn-webos` CLI, the Metro integration, the tests) needs
only Bun. Building and running an app needs the rest. macOS on Apple silicon is
the only host verified so far.

- [Bun](https://bun.sh) 1.4 or newer. CI runs the version in `.bun-version`.
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
  the example needs 22.18 or newer: its Metro config imports `withWebOS` from
  `src/`, so Node must strip TypeScript types on its own.
- For the simulator: [Lima](https://lima-vm.io) 2.0 or newer (`brew install
  lima`) and about 8 GB of free disk.

`rn-webos doctor` checks all of this and prints the fix for anything missing.

## Set up

```sh
git clone https://github.com/kromatv/react-native-webos.git
cd react-native-webos
bun install
```

The repository root is the package. `example/` is an Expo app in the same Bun
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

## Pull requests

- One change per pull request. It is squash-merged, so its title becomes the
  commit: conventional form, imperative, lowercase, such as
  `fix(input): map the Back key on webOS 8`.
- Code, comments and docs are in English.
- Comments are rare. Document exported API whose contract the signature does not
  show, and explain a platform constraint the code cannot. Nothing else.
- A file over 300 lines is split at a natural seam.
- Data from outside the process (files, the network, a TV, a child process) is
  parsed with zod.
- Say how you tested: the simulator, a TV (model and webOS version), or tests
  only.

## Releases

[Release Please](https://github.com/googleapis/release-please) cuts every
release from the squash commits on `main`, so a pull request's title is its line
in `CHANGELOG.md`:

| Title type | Changelog section | Bumps |
|------------|-------------------|-------|
| `feat` | Features | minor |
| `fix` | Bug fixes | patch |
| `perf` | Performance | patch |
| `revert` | Reverts | patch |
| `docs` | Documentation | patch |
| `refactor`, `test`, `build`, `ci`, `chore`, `style` | none | nothing on their own |

A `!` after the type or scope (`feat(cli)!: rename --device to --tv`) or a
`BREAKING CHANGE:` footer marks a breaking change, which bumps the major version
from 1.0.0 on and the minor version before it. The `Pull request title` check
refuses a title in any other form.

While the version is an alpha, every release bumps the alpha number
(`0.1.0-alpha.1`, `0.1.0-alpha.2`). Until a stable version is on npm, each one
goes to the `latest` tag, which `npx expo install` takes; after that,
prereleases go to `next`. Setting `prerelease` to `false` in
`release-please-config.json` ends the alpha: the next release is `0.1.0`.

How a release goes out:

1. Every push to `main` updates one pull request, `chore: release <version>`. It
   sets the version in `package.json` and `.release-please-manifest.json` and
   adds the release's section to `CHANGELOG.md`.
2. A maintainer merges it when `main` is worth releasing. The `Release` workflow
   then tags `v<version>` with a draft GitHub release, checks and packs the
   package, publishes it to npm with provenance, attaches the tarball to the
   release and publishes the release. A release that fails to reach npm stays a
   draft.

To fix a changelog line after its pull request merged, add the line as it should
read to that pull request's description, then run the `Release` workflow on
`main` from the Actions tab:

```text
BEGIN_COMMIT_OVERRIDE
fix(input): map the Back key on webOS 8
END_COMMIT_OVERRIDE
```

To choose the next version, end a squash commit's body with `Release-As: 1.0.0`.

### One-time setup

- A GitHub App opens the release pull request, because CI does not run on one
  opened with `GITHUB_TOKEN` until someone approves it. Create it under the
  organisation's Developer settings with no webhook and the repository
  permissions Contents, Issues and Pull requests set to read and write, and
  install it on this repository only. Save its client ID as the Actions variable
  `RELEASE_APP_CLIENT_ID` and a private key as the secret
  `RELEASE_APP_PRIVATE_KEY`.
- Add the app to the bypass list of the `release tags` ruleset, which lets only
  admins create `v*` tags.
- npm only lets a package that exists name a trusted publisher, so the first
  version, `0.1.0-alpha.0`, is published by hand. Then
  `npm trust github --file release.yml --repo kromatv/react-native-webos --env npm --allow-publish`
  makes the workflow its only publisher, and `npm access set mfa=publish` makes
  the package refuse tokens. No npm token is stored in GitHub.
- The `npm` environment deploys from `main` only.
- Make `Conventional title` a required check in the `main` ruleset.
