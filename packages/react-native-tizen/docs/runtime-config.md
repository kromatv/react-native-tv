# Runtime config

`res/react-native-tizen.json` is what the native runtime reads when the app
starts. `rn-tizen package` and `rn-tizen run` write it; you do not edit it.

```json
{
  "appKey": "main",
  "packageId": "com.example.myapp",
  "bundleFile": "index.tizen.hbc",
  "development": true,
  "log": { "host": "192.168.1.20", "port": 8099 }
}
```

| Field | Required | Meaning |
|-------|----------|---------|
| `appKey` | yes | The component to run: what `AppRegistry.registerComponent` registered. Expo's `registerRootComponent` registers `main`. |
| `packageId` | no | The Tizen package id. A development build names it when it connects its log stream and control link, so `rn-tizen run`, `logs`, `perf` and `screenshot` read one app when several on the TV stream to the same port. |
| `bundleFile` | yes | The bundle in `res/`, Hermes bytecode or plain JavaScript. |
| `bundleUrl` | no | Load the bundle from this URL (a Metro or Expo dev server) instead of `bundleFile`. The development runtime then connects to that server for Fast Refresh, reload, LogBox and logs in its terminal ([reference](reference/react-native-dev-tooling.md)). `rn-tizen run --dev-server <origin>`, `rn-tizen simulator run --dev-server <origin>` and the shift+m commands of `expo start` write Expo's virtual entry on that server. |
| `development` | no, default `false` | A fatal JavaScript error shows a red error screen with its stack, and Back closes the app. In a release build the app closes after logging the error, as React Native does on Android and iOS. |
| `log` | no | Stream every log line, JavaScript and native, to this host and port, where `rn-tizen run` or `rn-tizen logs` prints them. Retail TVs keep no readable system log, so a development build needs this. A native crash is streamed here too. |
| `resolution` | no, default `1080p` | `4k` sizes the window to the window manager's screen when it is 3840x2160 or larger (the config plugin's `resolution` prop). |
| `backgroundMedia` | no, default `false` | The manifest declares the `media` background category (expo-video's `supportsBackgroundPlayback`); without it the host warns when a player stays active in the background. |
| `voice` | no | The config plugin's `voice` prop: the shell registers with Samsung's voice service, and `searchUrl`, when present, is opened for a voice search no listener takes. |
| `preview` | no | The config plugin's `preview` prop: `kind` (`public` or `personal`), and for a personal preview `refreshMinutes` and `service`, the app id of the service app the shell starts at `OnPause`. |

The runtime refuses a file that is missing, larger than 64 KiB, or whose fields
have the wrong type, and logs which field is wrong. The .NET shell reads `voice`
and `preview` from the same file. See [tv-features.md](tv-features.md).
