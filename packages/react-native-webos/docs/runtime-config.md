# Runtime config

`res/react-native-webos.json` is what the native runtime reads when the app
starts. `rn-webos package` and `rn-webos run` write it; you do not edit it.

```json
{
  "appKey": "main",
  "packageId": "com.example.myapp",
  "bundleFile": "index.webos.hbc",
  "development": true,
  "log": { "host": "192.168.1.20", "port": 8099 }
}
```

| Field | Required | Meaning |
|-------|----------|---------|
| `appKey` | yes | The component to run: what `AppRegistry.registerComponent` registered. Expo's `registerRootComponent` registers `main`. |
| `packageId` | no | The webOS app id. A development build names it when it connects its log stream and control link, so `rn-webos run`, `logs`, `perf` and `screenshot` read one app when several on the TV stream to the same port. |
| `bundleFile` | yes | The bundle in `res/`, Hermes bytecode or plain JavaScript. |
| `bundleUrl` | no | Load the bundle from this URL (a Metro or Expo dev server) instead of `bundleFile`. The development runtime then connects to that server for Fast Refresh, reload, LogBox and logs in its terminal ([reference](reference/react-native-dev-tooling.md)). `rn-webos run --dev-server <origin>`, `rn-webos simulator run --dev-server <origin>` and the shift+m commands of `expo start` write Expo's virtual entry on that server. |
| `development` | no, default `false` | A fatal JavaScript error shows a red error screen with its stack, and Back closes the app. In a release build the app closes after logging the error, as React Native does on Android and iOS. |
| `log` | no | Stream every log line, JavaScript and native, to this host and port, where `rn-webos run` or `rn-webos logs` prints them. A retail TV does not let a developer read its system log, so a development build needs this. A native crash is streamed here too. |

The runtime refuses a file that is missing, larger than 64 KiB, or whose fields
have the wrong type, and logs which field is wrong.

## What else is in `res/`

The runtime finds its resource directory from its own path
(`/proc/self/exe`), never from `/media/developer`, where only a Developer Mode
install lives.

| File | |
|---|---|
| `index.webos.hbc` | The app's JavaScript, as Hermes bytecode. A build made with `--dev-server` carries an empty one. |
| `assets/` | The files the bundle requires, laid out as Metro serves them. |
| `icudt78l.dat` | ICU's data, trimmed to the languages of the config plugin's `locales` prop: what `Intl` formats with and what text is broken into lines with. |
| `cacert.pem` | Mozilla's root certificates, which the runtime's HTTPS trusts. |
| `app.config` | The app's public Expo config, which `expo-constants` reads. |
| `THIRD_PARTY_NOTICES.md` | The licences of what the package holds. |
