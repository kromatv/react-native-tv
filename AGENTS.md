# Agent instructions

One repository, three npm packages. Work in the package a change belongs to and
read its `CLAUDE.md` and `AGENTS.md` first: they say what the package is, which
reference file covers which code, and how to check a change.

| Directory | Package | What |
|-----------|---------|------|
| `packages/react-native-tv` | `@kromatv/react-native-tv` | one setup for every TV platform; holds no platform code |
| `packages/react-native-tizen` | `@kromatv/react-native-tizen` | Samsung Tizen TVs: the runtime, the `rn-tizen` CLI, the simulator |
| `packages/react-native-webos` | `@kromatv/react-native-webos` | LG webOS TVs: the runtime, the `rn-webos` CLI, the simulator |

## Across packages

- No package imports another. `react-native-tv` finds a platform package through
  `reactNativeTvPlatform` in its `package.json`. Nothing here may depend on
  KROMA or any other app.
- A pull request's title becomes its squash commit and, for `feat`, `fix`,
  `perf`, `revert` and `docs`, its line in the `CHANGELOG.md` of every package
  whose files it changes. Keep such a pull request to one package.
  [`CONTRIBUTING.md`](CONTRIBUTING.md), Releases, has the types and what they
  bump.
- English for code, comments, identifiers, docs and commit messages. Never an
  em dash, no en dash as a connector, no curly quotes.
- Never commit signing material, a generated `tizen/` or `webos/` project, build
  output or anything under `node_modules/`.

## Checks

From the root they run in every package, from a package's directory in that one:

```sh
bun run typecheck
bun run test
bun run check
bun run build
shellcheck -x scripts/*.sh
```

## Root files

| Path | What |
|------|------|
| `.github/workflows/ci.yml` | finds the changed packages (`scripts/changed-packages.sh`) and calls `package.yml` for each; `CI result` is the one required check |
| `.github/workflows/release.yml` | Release Please, then pack and publish each released package |
| `release-please-config.json`, `.release-please-manifest.json` | one entry per package |
| `scripts/import-history.sh` | how the Tizen and webOS repositories were imported with their history |
