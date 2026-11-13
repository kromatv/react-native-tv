# Contributing

Bug reports, fixes and new platform coverage are welcome. Open an issue before a
large change so we can agree on the approach first.

## The repository

Three npm packages live in `packages/`. Each has its own README, its own
reference in `docs/reference/` and its own guide:

| Directory | Package | Guide |
|-----------|---------|-------|
| `packages/react-native-tv` | `@kromatv/react-native-tv` | [`AGENTS.md`](packages/react-native-tv/AGENTS.md) |
| `packages/react-native-tizen` | `@kromatv/react-native-tizen` | [`CONTRIBUTING.md`](packages/react-native-tizen/CONTRIBUTING.md) |
| `packages/react-native-webos` | `@kromatv/react-native-webos` | [`CONTRIBUTING.md`](packages/react-native-webos/CONTRIBUTING.md) |

`@kromatv/react-native-tv` lists the two platform packages in its
`dependencies`, as a version range, and imports nothing from them: it finds a
platform by what the platform's `package.json` declares.

## Set up

```sh
git clone https://github.com/kromatv/react-native-tv.git
cd react-native-tv
bun install
```

[Bun](https://bun.sh) 1.4 or newer. CI runs the version the root `package.json`
names. `bun install` builds each package's `dist/`.

From the root, a check runs in every package. From a package's directory, it
runs in that one:

```sh
bun run typecheck
bun run test
bun run check        # oxlint and oxfmt; bun run check:fix rewrites
bun run build
```

Building and running an app on a TV or in a simulator needs more. The Tizen and
webOS guides say what.

## Pull requests

- One change per pull request, in one package when it can be. It is
  squash-merged, so its title becomes the commit: conventional form,
  imperative, lowercase, such as `fix(input): map the Back key on Tizen 8`.
- CI checks the packages a pull request touches. A change to the root
  `package.json`, `bun.lock` or the workflows checks all three.
- Code, comments and docs are in English.
- Comments are rare. Document exported API whose contract the signature does not
  show, and explain a platform constraint the code cannot. Nothing else.
- A file over 300 lines is split at a natural seam.
- Data from outside the process (files, the network, a TV, a child process) is
  parsed with zod.
- Say how you tested: a simulator, a TV (model and OS version), or tests only.

## Releases

[Release Please](https://github.com/googleapis/release-please) cuts every
release from the squash commits on `main`. Each package has its own version,
its own `CHANGELOG.md` and its own release pull request. A commit counts for a
package when it changes a file in that package's directory, so a pull request's
title is its line in the changelog of every package it touches:

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
from 1.0.0 on and the minor version before it. The `Conventional title` check
refuses a title in any other form.

While a package's version is an alpha, every release bumps the alpha number
(`0.1.0-alpha.1`, `0.1.0-alpha.2`). Until a stable version is on npm, each one
goes to the `latest` tag, which `npx expo install` takes; after that,
prereleases go to `next`. Setting `prerelease` to `false` for a package in
`release-please-config.json` ends its alpha: the next release is `0.1.0`.

How a release goes out:

1. Every push to `main` updates one pull request per package with something to
   release, `chore: release <package> <version>`. It sets the version in the
   package's `package.json` and in `.release-please-manifest.json`, and adds
   the release's section to the package's `CHANGELOG.md`.
2. A maintainer merges it when the package is worth releasing. The `Release`
   workflow then tags `<package>-v<version>` with a draft GitHub release, checks
   and packs the package, publishes it to npm with provenance, attaches the
   tarball to the release and publishes the release. A release that fails to
   reach npm stays a draft.

To fix a changelog line after its pull request merged, add the line as it should
read to that pull request's description, then run the `Release` workflow on
`main` from the Actions tab:

```text
BEGIN_COMMIT_OVERRIDE
fix(input): map the Back key on Tizen 8
END_COMMIT_OVERRIDE
```

To choose the next version, end a squash commit's body with `Release-As: 1.0.0`.

### One-time setup

- A GitHub App opens the release pull requests, because CI does not run on one
  opened with `GITHUB_TOKEN` until someone approves it. Create it under the
  organisation's Developer settings with no webhook and the repository
  permissions Contents, Issues and Pull requests set to read and write, and
  install it on this repository. Save its client ID as the Actions variable
  `RELEASE_APP_CLIENT_ID` and a private key as the secret
  `RELEASE_APP_PRIVATE_KEY`.
- Add the app to the bypass list of the `release tags` ruleset, which lets only
  admins create `react-native-*-v*` tags.
- npm only lets a package that exists name a trusted publisher, so the first
  version of each package was published by hand. Then, once per package,
  `npm trust github @kromatv/<package> --file release.yml --repo kromatv/react-native-tv --env npm --allow-publish`
  made the workflow its only publisher, and `npm access set mfa=publish` makes
  the package refuse tokens. No npm token is stored in GitHub.
- The `npm` environment deploys from `main` only.
- Make `CI result` and `Conventional title` required checks in the `main`
  ruleset.
