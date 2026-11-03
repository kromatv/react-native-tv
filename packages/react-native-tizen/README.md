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
  with, from its Package Manager, `TIZEN-9.0-NativeAppDevelopment-CLI` and the
  Samsung Certificate Extension.
- LLVM, CMake, Ninja and .NET:

  ```sh
  brew install llvm lld cmake ninja dotnet ccache
  ```

- A Samsung certificate for your TV: Tizen Studio, Tools, Certificate Manager,
  a Samsung profile with your TV's DUID. Make it the active profile.
- [Node.js](https://nodejs.org) 22.13 or newer, which runs the CLI and Expo.

### 2. Add the package

```sh
npm install @kromatv/react-native-tizen
```

Teach Metro the `tizen` platform in `metro.config.js`:

```js
const { getDefaultConfig } = require('expo/metro-config');
const { withTizen } = require('@kromatv/react-native-tizen/metro');

module.exports = withTizen(getDefaultConfig(__dirname));
```

Give the app a Tizen package id in `app.json`:

```json
{
  "expo": {
    "tizen": { "package": "com.example.myapp" }
  }
}
```

### 3. Put the TV in Developer mode

On the TV, open Apps, type `12345` on the remote, turn Developer mode on with
your computer's IP address, then restart the TV.

### 4. Check, then run

```sh
npx rn-tizen doctor
npx rn-tizen run --device 192.168.1.50
```

`doctor` names the exact fix for anything missing. The first `run` builds the
native runtime, which takes a few minutes; later runs only rebuild what
changed. It installs the app, launches it and prints its logs until Ctrl+C.

Store packages need a release build, which `package` makes; `--dev` makes a
development one that streams its logs:

```sh
npx rn-tizen package
npx rn-tizen package --dev
```

Publishing to Samsung's store needs a Seller Office partner group, because the
package carries a native library.

## Documentation

- [The CLI](docs/cli.md): every command, its options, the `expo.tizen` config,
  and where builds land.

## Licence

MIT. What the runtime and the packages it builds carry from other projects,
and under which licences, is in [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).

Not affiliated with or endorsed by Samsung, the Linux Foundation, Meta or Expo.
Samsung, Tizen, Smart Hub, React Native and Expo are trademarks of their owners.
