# Reference

What this package builds on, written down so nobody implements a behaviour of
Expo CLI, Metro or a package manager from memory. Each file cites a source for
every fact and ends with what the facts mean for this package.

| File | Read it before touching |
|------|------------------------|
| [expo-cli.md](expo-cli.md) | `src/expo-cli/`, `src/expo-patch/`: where Expo CLI keeps its commands, how it picks one, what `run` accepts, how each package manager patches a package |
| [expo-config-and-start.md](expo-config-and-start.md) | `src/config/`, `src/start/`: how Expo applies config plugins, where the platform packages read their settings, what a dev tools plugin does inside `expo start`, the keys |

How Expo loads a Metro config, config plugins and dev tools plugins is in the
reference of the platform packages:
[`expo-cli-integration.md`](https://github.com/kromatv/react-native-tv/blob/main/packages/react-native-webos/docs/reference/expo-cli-integration.md).

## How a fact is written

- **Cited.** A file and line of the source it was read in, or a link to the
  documentation.
- **Tagged with how it is known:** read in source, observed (say where and with
  which versions), or documented.
- **Dated** when it is an observation.
