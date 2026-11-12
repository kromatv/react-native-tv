import { tvPlatformPackages } from '../platforms/tv-platform-packages.ts';
import type { CliIo } from './cli-io.ts';

/** `rn-tv platforms`: each TV platform package of the app, and what it gives. */
export function platformsCommand(io: CliIo): number {
  const packages = tvPlatformPackages(io.cwd);
  if (packages.length === 0) {
    io.out(
      'This app depends on no TV platform package, such as @kromatv/react-native-tizen or @kromatv/react-native-webos.\n',
    );
    return 0;
  }
  for (const { name, platform, cli } of packages) {
    const run =
      platform !== null && cli !== null
        ? `run:${platform}`
        : 'Metro only: this version names no platform or command to run';
    io.out(`${name}  ${run}\n`);
  }
  return 0;
}
