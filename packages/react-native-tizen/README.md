# react-native-tizen

React Native and Expo on Samsung Tizen TVs, natively: Hermes and Fabric run on
the TV with no browser in between. Your Expo app gets one more platform,
`tizen`, and `rn-tizen` turns it into a signed package and runs it.

On the TV, a small .NET app (Samsung's store takes web and .NET apps only) loads
`libreact-native-tizen.so`, which holds Hermes, React Native's C++ core and the
Tizen host. Your JavaScript ships as Hermes bytecode.

Verified on a Samsung TV running Tizen 9.0 (armv7), built from macOS.

## Quick start

### 1. Install the tools

- [Tizen Studio](https://developer.tizen.org/development/tizen-studio/download)
  with, from its Package Manager, `TIZEN-9.0-NativeAppDevelopment-CLI` (the
  rootstrap and GCC runtime the native build links against) and the Samsung
  Certificate Extension. `rn-tizen` never runs Tizen Studio's `sdb` or `tizen`
  tools: it reaches the TV and signs the package itself.
- LLVM, CMake, Ninja and .NET:

  ```sh
  brew install llvm lld cmake ninja dotnet ccache
  ```

- A Samsung certificate for your TV, once: in Tizen Studio, Tools, Certificate
  Manager, create a Samsung profile with your TV's DUID and make it the active
  profile. Samsung issues both its certificates, and a retail TV takes no
  other. `rn-tizen` reads the profile from there.
- [Node.js](https://nodejs.org) 22.13 or newer, which runs the CLI and Expo.

### 2. Add the package

```sh
npx expo install @kromatv/react-native-tizen
npx rn-tizen init
```

`rn-tizen init` gives the config plugin Expo listed in `app.json` your app's
Tizen package id. It asks for one, suggesting your Android package or one made
from the slug. It also writes `metro.config.mjs`, which teaches Metro the
`tizen` platform:

```js
import { getDefaultConfig } from 'expo/metro-config';
import { withTizen } from '@kromatv/react-native-tizen/metro';
export default withTizen(getDefaultConfig(import.meta.dirname));
```

An app that already has a Metro config keeps it: `init` wraps its export with
`withTizen`, or prints the two lines to add when it cannot change the file
safely. Running `init` again changes nothing.

### 3. Put the TV in Developer mode

On the TV, open Apps, type `12345` on the remote, turn Developer mode on with
your computer's IP address, then restart the TV.

### 4. Run it from `expo start`

```sh
npx expo start
```

Press **shift+m** ("more tools") and pick `@kromatv/react-native-tizen`:

- **Run on TV** builds a development build, installs it on the TV and launches
  it. It loads its JavaScript from the dev server you are running. Leave the
  address empty to use the TV you used last.
- **Run in simulator** does the same on the virtual TV of
  [the simulator](simulator/README.md).
- **Find TVs** lists the Samsung TVs on your network, with their Developer
  mode, sdb and DUID.
- **Create certificate** makes an author certificate.

Each command prints the address of a page inside the dev server, where you can
follow its progress and logs. The first run builds the native runtime, which
takes a few minutes; later runs only rebuild what changed. Run on TV and Run
in simulator check the Metro config first, and say to run `npx rn-tizen init`
when it has no `withTizen`. `npx rn-tizen doctor` names the exact fix for
anything a build needs.

Expo has no `expo run:tizen` and no single-key shortcut for Tizen, and
`expo prebuild` and `expo export --platform tizen` do not know the platform.
For a script beside Expo's own, add one to `package.json`:

```json
{
  "scripts": {
    "android": "expo run:android",
    "tizen": "rn-tizen run"
  }
}
```

`npm run tizen` packages the app with its JavaScript inside, installs it on the
only TV used before, launches it and prints its logs until Ctrl+C.

Store packages need a release build, which `package` makes; `--dev` makes a
development one that streams its logs:

```sh
npx rn-tizen package
npx rn-tizen package --dev
```

Publishing to Samsung's store needs a Seller Office partner group, because the
package carries a native library.

## Documentation

- [The CLI](docs/cli.md): every command, its options, the config plugin's props,
  `expo start` integration, and where builds land. `rn-tizen devices` finds the TVs on your network,
  `rn-tizen certificate` manages signing profiles, and `rn-tizen install`
  signs and installs any `.tpk` or `.wgt`.
- [The `tizen` API](docs/tizen-api.md): the same, from Node, for other tools.

## Licence

MIT. What the runtime and the packages it builds carry from other projects,
and under which licences, is in [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).

Not affiliated with or endorsed by Samsung, the Linux Foundation, Meta or Expo.
Samsung, Tizen, Smart Hub, React Native and Expo are trademarks of their owners.
