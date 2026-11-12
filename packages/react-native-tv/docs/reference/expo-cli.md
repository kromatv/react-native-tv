# Expo CLI: its command table, and patching it

Where `npx expo <command>` finds its commands, why a package cannot add one,
and how a patch of `@expo/cli` is written and applied with each package
manager. Checked on 2026-11-12.

| Tag | Meaning |
|---|---|
| **S** | Read in source: `@expo/cli` 58.1.19 as installed by `expo` 58.0.23, under `build/` |
| **O** | Observed on 2026-11-12, on an arm64 Mac with Node 26.5.0: an app in a temporary directory with `expo` 58.0.23 (`@expo/cli` 58.1.20), `react-native-tvos` 0.88.0-0, this package from its packed tarball, and `@kromatv/react-native-tizen` 0.1.0-alpha.4 and `@kromatv/react-native-webos` 0.1.0-alpha.2 with `platform` and `cli` added to their `reactNativeTvPlatform`. Sections 1 to 3 ran with Bun 1.4.2. Sections 3 to 6 name the manager each fact was seen with: Bun 1.4.2, pnpm 11.2.2 and 12.18.0, Yarn 4.18.1 with `nodeLinker: node-modules`, Yarn 1.22.22 and 2.4.3 through Corepack 0.36.0, npm 11.17.0. Their upgrade cases started from `expo` 58.0.22 with `@expo/cli` held at 58.1.18 |
| **D** | Read in the documentation linked at the end, or in the help a tool prints |

## 1. The command table

| Fact | Tag | Source |
|---|---|---|
| `expo`'s command is `require('@expo/cli')`, and `@expo/cli` exports `./build/src/index.js` as its main module. Its own bin, `expo-internal`, requires the same file. | S | `expo` 58.0.23 `bin/cli:3`; `@expo/cli` `package.json:20-22, 30-36`, `bin/cli.js:3` |
| The commands are the keys of one object, `commands`, each a function that imports the command and gives the function to run it: `'run:ios': ()=>import('../src/run/ios/index.js').then((i)=>i.expoRunIos)`. | S | `build/src/index.js:47-74` |
| Right after the table, the arguments are parsed. The first is a command when it is a key of `commands`; anything else is taken as an argument of `start`, the default. So `expo run:webos` is `expo start run:webos`. | S | `build/src/index.js:46, 75-91` |
| The command runs as `commands[command]().then((exec) => { exec(commandArgs) ... })`, with the arguments after the command's name; `--help` is pushed onto them. | S | `build/src/index.js:241-250` |
| `expo --help` prints `start`, `export` and every key of `commands` its code does not name, then `run:ios, run:android, prebuild`: a key added to the table is listed on the first line. | S | `build/src/index.js:126-142` |
| After a command runs, Expo records its name as telemetry unless `EXPO_NO_TELEMETRY` is set. | S | `build/src/index.js:251-257` |
| Nothing between the start of the file and the lookup reads the project: no file of the app is loaded before the command is chosen. A package therefore cannot add a command without a change to this file. | S | `build/src/index.js:1-91` |
| `expo run <platform>` takes `android` or `ios`, asks which when none is given, and throws `Unsupported platform` for any other. | S | `build/src/run/index.js:39-76` |
| Unpatched, `npx expo run:webos --help` printed Expo's general help. | O | |

## 2. The patch

| Fact | Tag | Source |
|---|---|---|
| With a block inserted before `const args = (0, _arg().default)({` that requires this package's `expo-cli` module, resolved from `process.cwd()`, and passes it `commands`: `npx expo run:webos --help` and `npx expo run tizen --help` printed the help of `rn-webos run` and `rn-tizen run`; `npx expo run:tizen --device 192.0.2.1` exited with 1, the platform command's code; `npx expo --help` listed `start, export, run:tizen, run:webos`; `npx expo run:android --help` and `npx expo config` were Expo's. | O | |
| Run from a directory whose project does not depend on this package, the patched CLI behaved as unpatched. | O | |
| In the example app of react-native-tizen at its commit `35056e6` plus the `platform` and `cli` declaration, with Expo CLI 58.1.20 patched through Bun: `npx expo run:tizen --device <ip> --no-logs` built the development runtime, signed, installed and launched the app, and exited with 0. A screenshot of the TV then showed the app's home screen, up for 12 s. | O | GQ75LS03DAUXZG (The Frame, 2024, Tizen 9.0), 2026-11-12 |
| Expo CLI is CommonJS and the block uses `require` on this package's ES module. | S, O | `build/src/index.js:1-40` |
| `require()` loads an ES module that has no top-level `await`: behind no flag from Node 22.12.0, and with no warning from 22.13.0. Expo CLI warns below Node 22.13.0. | D, S | [node-require-esm]; `build/src/index.js:115-124` |

## 3. Bun's patch feature

| Fact | Tag | Source |
|---|---|---|
| `bun patch @expo/cli@58.1.20`, run where `bun.lock` is, printed the directory to edit, `node_modules/@expo/cli`, and the command to finish: `bun patch --commit 'node_modules/@expo/cli'`. | O | Bun 1.4.2 |
| `bun patch --commit` wrote `patches/@expo%2Fcli@58.1.20.patch`, a diff of `build/src/index.js`, added `"patchedDependencies": { "@expo/cli@58.1.20": "patches/@expo%2Fcli@58.1.20.patch" }` to `package.json`, listed the patch in `bun.lock`, and reinstalled. | O | Bun 1.4.2 |
| The commit rewrites `package.json`: an `overrides` object written on one line came back on several. | O | Bun 1.4.2 |
| After `rm -rf node_modules` and `bun install`, the installed `@expo/cli` had the block. | O | Bun 1.4.2 |
| With an entry for `@expo/cli` 58.1.18 and 58.1.20 installed, `bun install` printed nothing about the entry and exited with 0. The entry and the patch file stayed, and `bun.lock` listed no patch. | O | Bun 1.4.2 |
| A commit for the new version added its entry beside the old one. | O | Bun 1.4.2 |
| With the old entry cut from the text of `package.json` and its file deleted, `bun install` from an empty `node_modules` applied the remaining patch. | O | Bun 1.4.2 |
| `bun patch <package>@<version>`: "Prepare a package for patching, or generate and save a patch." Its options for patches are `--commit` and `--patches-dir`: none removes a patch. | D | `bun patch --help`, Bun 1.4.2 |
| An Expo upgrade does not always change `@expo/cli`: with 58.1.19 in the lockfile, moving `expo` from 58.0.22 (`@expo/cli` `^58.1.18`) to 58.0.23 (`^58.1.19`) kept 58.1.19, and its patch stayed in use. | O | Bun 1.4.2 |

## 4. pnpm's patch feature

| Fact | Tag | Source |
|---|---|---|
| `pnpm patch @expo/cli@58.1.20` printed, on stdout, `Patch: You can now edit the package at:`, the directory `<app>/node_modules/.pnpm_patches/@expo/cli@58.1.20`, and `pnpm patch-commit '<that directory>'`. 12.18.0 prints a progress line first. | O | pnpm 11.2.2, 12.18.0 |
| That directory stays after the commit, and a second `pnpm patch` of the package fails with `ERR_PNPM_EDIT_DIR_NOT_EMPTY`. | O | pnpm 11.2.2 |
| `pnpm patch --edit-dir <dir>` extracts to `<dir>` instead, a directory that is new or empty. | O, D | pnpm 11.2.2, 12.18.0; `pnpm patch --help` |
| The copy `pnpm patch` prepares has the patch the app has for the package already. `--ignore-existing` starts without it. | O, D | pnpm 11.2.2; `pnpm patch --help` |
| `pnpm patch-commit <dir>` wrote `patches/@expo__cli@58.1.20.patch`, added `patchedDependencies: '@expo/cli@58.1.20': patches/@expo__cli@58.1.20.patch` to `pnpm-workspace.yaml`, recorded the patch's hash in `pnpm-lock.yaml`, and installed. `package.json` was left byte for byte. | O | pnpm 11.2.2, 12.18.0 |
| Writing `pnpm-workspace.yaml`, 11.2.2 kept the comments and set the indentation of every key to two spaces. 12.18.0 left the other lines as they were. | O | pnpm 11.2.2, 12.18.0 |
| The patch has a second part, `deleted file mode 100644` for `LICENSE-third-party`: `@expo/cli` ships that file and its `files` field (`main.js`, `internal`, `static`, `build`) does not list it. The patched package is installed without it. | O | pnpm 11.2.2, 12.18.0 |
| After `rm -rf node_modules` and `pnpm install`, the installed `@expo/cli` had the block. | O | pnpm 11.2.2, 12.18.0 |
| With an entry for `@expo/cli` 58.1.18 and an install that resolves 58.1.20, `pnpm install` failed with `ERR_PNPM_UNUSED_PATCH`, "The following patches were not used: @expo/cli@58.1.18", and exit code 1. The lockfile and `node_modules` stayed as before the upgrade, the old Expo CLI still installed and patched. | O | pnpm 11.2.2, 12.18.0 |
| In that state `pnpm add expo@<version>` printed the same error and left `package.json` as it was. | O | pnpm 11.2.2 |
| In that state `pnpm exec <command>` printed the same error, warned that "The install that runs before scripts failed", and ran the command. | O | pnpm 12.18.0 |
| With `allowUnusedPatches: true` in `pnpm-workspace.yaml` the install went through and printed `[WARN] The following patches were not used: @expo/cli@58.1.18`. The entry and the file stayed. | O | pnpm 12.18.0 |
| `pnpm patch-remove @expo/cli@58.1.18` removed the entry, the `patchedDependencies` key with its last entry, the patch file and the emptied `patches` directory, and installed. It ran while installs were refused, and that install brought the new `@expo/cli`. | O | pnpm 11.2.2, 12.18.0 |
| `pnpm config get patchedDependencies --json` printed the entries as a JSON object, each with the absolute path of its patch file. With no patch it printed nothing (11.2.2) or `null` (12.18.0). | O | pnpm 11.2.2, 12.18.0 |
| `pnpm install` exits with 1 and `ERR_PNPM_IGNORED_BUILDS` while a dependency's build script is neither allowed nor refused under `allowBuilds`. | O | pnpm 11.2.2 |

## 5. Yarn 2 and later

| Fact | Tag | Source |
|---|---|---|
| A lockfile of Yarn 1 starts with `# THIS IS AN AUTOGENERATED FILE. DO NOT EDIT THIS FILE DIRECTLY.` and `# yarn lockfile v1`. One of Yarn 2.4.3 or 4.18.1 has `__metadata:` after two comment lines and no such header. | O | Yarn 1.22.22, 2.4.3, 4.18.1 |
| Yarn 4.18.1 installed a project that had no `.yarnrc.yml` and no `packageManager` field, and created neither. Corepack's `yarn` in a project with no `packageManager` field ran Yarn 1 and added no field. | O | Yarn 4.18.1, 1.22.22, Corepack 0.36.0 |
| `yarn patch @expo/cli@58.1.20` failed with "No package found in the project for the given locator". `yarn patch --json @expo/cli@npm:58.1.20` printed one line: `{"locator":"@expo/cli@npm:58.1.20","path":"<tmp>/xfs-<id>/user"}`, the directory to edit. | O | Yarn 4.18.1 |
| `yarn patch-commit -s <path>` printed nothing, wrote `.yarn/patches/@expo-cli-npm-58.1.20-b9cf9fdd52.patch`, a diff of `build/src/index.js` only, and added `"resolutions": { "@expo/cli@npm:^58.1.19": "patch:@expo/cli@npm%3A58.1.20#~/.yarn/patches/@expo-cli-npm-58.1.20-b9cf9fdd52.patch" }` to `package.json`. The key is the range `expo` asks for. | O | Yarn 4.18.1 |
| The commit does not install: `node_modules` had the block only after `yarn install`. | O | Yarn 4.18.1 |
| After `rm -rf node_modules` and `yarn install --immutable`, the installed `@expo/cli` had the block. | O | Yarn 4.18.1 |
| The patch file is named after the version: a commit for the same version under another range added a second entry that names the same file. | O | Yarn 4.18.1 |
| When `expo` asked for another range, `yarn install` printed nothing about the entry, exited with 0, dropped the patched package from `yarn.lock` and installed `@expo/cli` unpatched. The entry and the file stayed. The entry can be for the very version installed: `^58.1.18` and `^58.1.19` both resolved to 58.1.20. | O | Yarn 4.18.1 |
| With such an entry cut from the text of `package.json` and its file deleted, `yarn install --immutable` passed. | O | Yarn 4.18.1 |
| In an app that has its own patch of the package, `yarn patch --json @expo/cli@npm:58.1.20`, with `--update` or not, extracted the package as published, without that patch. `yarn patch --update --json @expo/cli` extracted it with the patch. | O | Yarn 4.18.1 |
| In an app with no patch of the package, `yarn patch --update --json @expo/cli` failed with "No package found in the project for the given locator". | O | Yarn 4.18.1 |
| A commit made from `--update` left the app's patch file as it was, wrote the new change alone to `.yarn/patches/@expo-cli-patch-e6483134a3.patch`, and replaced the resolution with one that names the first patch inside the second: `patch:@expo/cli@patch%3A@expo/cli@npm%253A58.1.20%23~/.yarn/patches/<first>.patch%3A%3Aversion=58.1.20&hash=1c9d47#~/.yarn/patches/@expo-cli-patch-e6483134a3.patch`. | O | Yarn 4.18.1 |
| `yarn patch` takes `--update` and `--json`, `yarn patch-commit` takes `--save`: neither removes a patch. | D | `yarn patch --help`, `yarn patch-commit --help`, Yarn 4.18.1 |

## 6. npm and Yarn 1: no patch feature

| Fact | Tag | Source |
|---|---|---|
| `npm patch` printed `Unknown command: "patch"`. `yarn patch` printed `error Command "patch" not found.` | O | npm 11.17.0, Yarn 1.22.22 |
| npm installed `@expo/cli` under `node_modules/expo/node_modules/@expo/cli`. Yarn 1, Yarn 4 and Bun installed it at `node_modules/@expo/cli`, pnpm under `node_modules/.pnpm/@expo+cli@<version>_<hash>/node_modules/@expo/cli`. | O | npm 11.17.0, Yarn 1.22.22, 4.18.1, Bun 1.4.2, pnpm 11.2.2 |
| The `postinstall` script of the project ran after `npm install` with no argument, also with nothing to install, and after `npm ci`, in the project's directory and with `node_modules/.bin` on `PATH`. | O | npm 11.17.0 |
| It did not run after `npm install <package>`, `npm uninstall <package>` or `npm update`, nor with `--ignore-scripts`. | O | npm 11.17.0 |
| With Yarn 1 it ran after `yarn install`, also when "Already up-to-date", after `yarn install --frozen-lockfile` and after `yarn add <package>`. It did not run after `yarn remove <package>`. | O | Yarn 1.22.22 |
| In a workspace, an install from the root ran the root's `postinstall` and not the one of a package in it. `npm install` from inside the package ran the root's. `yarn install` from inside the package ran the package's and not the root's. | O | npm 11.17.0, Yarn 1.22.22 |
| With the block written into the installed file, `npm install` with nothing to change left the file as it was. | O | npm 11.17.0 |
| An install from an empty `node_modules`, and one that changed the version of `@expo/cli`, brought the file back without the block, before `postinstall` ran. | O | npm 11.17.0, Yarn 1.22.22 |
| Yarn 1 did not finish resolving a `file:` tarball that holds directory entries, as `tar` writes them: it stayed at `[1/4] Resolving packages...` on a full core for over ten minutes. The same files in a tarball without directory entries, as `bun pm pack` writes one, installed in seconds. | O | Yarn 1.22.22 on Node 26.5.0 |

## What this means for this package

- The patch is one insertion, between the command table and the argument
  parser, found by the parser's line. `addExpoCliHook` refuses a file that has
  no such line, more than one, or the table after it, and `rn-tv expo-patch`
  then says the layout is unknown and changes nothing.
- The inserted block resolves `@kromatv/react-native-tv/expo-cli` from the
  current directory and does nothing when that fails, so a patched Expo CLI in
  a workspace is inert for the apps that do not use this package. A failure
  while loading or running the extension is a warning.
- `extendExpoCli` adds `run:<platform>` only where the table has no such key,
  and wraps `run` so `expo run <platform>` reaches a TV platform and every
  other platform reaches Expo.
- A platform command exits the process with its own code, before Expo records
  the command.
- `rn-tv expo-patch` works in the directory of the nearest lockfile, and the
  lockfile names the package manager. `yarn.lock` is Yarn 1 when it has the
  `# yarn lockfile v1` header and Yarn 2 or later when it does not: nothing
  else is there in every project.
- Every copy of the entry file is patched by writing a new file and renaming
  it over the old one, so a file the manager links to its store is never
  modified. A copy that has the block already is left alone, because pnpm and
  Yarn's `--update` hand out a copy with the app's patch applied.
- Bun: `bun patch`, an edit of the directory Bun names, `bun patch --commit`.
- pnpm: `pnpm patch --edit-dir` into a new temporary directory, so a
  directory left under `node_modules/.pnpm_patches` is never in the way, then
  `pnpm patch-commit`. pnpm writes `pnpm-workspace.yaml` itself. The licence
  file the patch drops from the installed `@expo/cli` is pnpm's doing and is
  left as pnpm wrote it.
- Yarn 2 and later: `yarn patch --json @expo/cli@npm:<version>`, an edit of
  the path it prints, `yarn patch-commit -s`, `yarn install`. When
  `resolutions` has a patch of `@expo/cli` that is not ours, the first command
  is `yarn patch --update --json @expo/cli` instead, which builds on that
  patch.
- npm and Yarn 1: the block goes into the installed file, and `rn-tv
  expo-patch` joins the app's `postinstall` script, after what the script runs
  already, unless one of the app's scripts runs it. The script goes in the app's
  own `package.json`: in a workspace an install from the root does not run it,
  and `rn-tv expo-patch` says so. With npm, `npm install <package>` runs no
  `postinstall`, so an Expo upgrade made that way needs `rn-tv expo-patch`
  again.
- A patch left by an upgrade is ours when its file contains
  `@kromatv/react-native-tv/expo-cli`. `rn-tv expo-patch` removes ours and
  never another:
  - Bun says nothing about one. Its entry leaves `patchedDependencies` by an
    edit of the text of `package.json` that touches nothing else, and its file
    is deleted.
  - Yarn says nothing about one. Its entry leaves `resolutions` the same way.
    An entry is old when it is for another version, and also for the installed
    version when the installed file has no block, since Yarn keys entries by
    range. A patch stacked on the app's own patch is not removed: its entry
    carries both.
  - pnpm refuses to install with one, and the install it refuses is the one
    that would bring the new `@expo/cli`, so the old Expo CLI is still
    installed and patched and there is nothing for `rn-tv expo-patch` to see.
    The app runs `pnpm patch-remove @expo/cli@<old version>` first, which
    `rn-tv expo-patch` prints. Where `allowUnusedPatches` lets the install
    through, `rn-tv expo-patch` runs `pnpm patch-remove` itself.
  - npm and Yarn 1 leave nothing behind: there is no patch file.
  - With Bun and pnpm a patch holds every change the app made to the package,
    so one that has our block and the app's own changes counts as ours.
- Not known yet: whether a Yarn `resolutions` entry holds `@expo/cli` at the
  patched version when a newer one fits the range `expo` asks for; what
  `yarn patch --update @expo/cli` does in a project with two versions of
  `@expo/cli`; pnpm before 11; and any of this on Windows.

## Sources

The files of sections 1 and 2, under
`node_modules/.bun/@expo+cli@58.1.19*/node_modules/@expo/cli/` and
`node_modules/.bun/expo@58.0.23*/node_modules/expo/` of
`@kromatv/react-native-webos` at its commit `3568aa2`. Sections 3 to 6 are
observations and the help each manager prints.

[node-require-esm]: https://nodejs.org/docs/latest-v22.x/api/modules.html#loading-ecmascript-modules-using-require
