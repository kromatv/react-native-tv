import { mkdir, writeFile } from 'node:fs/promises';
import { dirname, join, resolve } from 'node:path';
import { cacheDir } from '../cache-dir.ts';
import { captureScreen } from '../capture/capture-screen.ts';
import { encodePng } from '../capture/png.ts';
import { COMMON_OPTIONS, parseCommandArgs, PROJECT_OPTIONS, type Command } from '../cli/command.ts';
import { deviceAddress } from '../cli/device-option.ts';
import { parsePort } from '../cli/log-options.ts';
import { PACKAGE_OPTIONS, targetPackage } from '../cli/package-option.ts';
import { Reporter } from '../cli/reporter.ts';

const OPTIONS = {
  device: {
    type: 'string',
    short: 'd',
    placeholder: '<ip>',
    description: 'The TV whose app to capture (default: the only one connected)',
  },
  out: {
    type: 'string',
    placeholder: '<file.png>',
    description: 'Where to save it (default: tv/<ip>/screenshot.png in the cache)',
  },
  port: {
    type: 'string',
    placeholder: '<port>',
    description: 'The port the app streams its logs to, as packaged (default: 8099)',
  },
  ...PACKAGE_OPTIONS,
  ...PROJECT_OPTIONS,
  ...COMMON_OPTIONS,
} as const;

function defaultPath(device: string | null): string {
  return join(cacheDir(), 'tv', device ?? 'any', 'screenshot.png');
}

/** `rn-tizen screenshot`: saves what a development build draws as a PNG. */
export const screenshotCommand: Command = {
  name: 'screenshot',
  summary: "Save what a development build on a TV draws as a PNG (the app's UI only)",
  options: OPTIONS,
  examples: [
    'rn-tizen screenshot --device 192.168.1.50 --out shot.png',
    'rn-tizen screenshot',
    'rn-tizen screenshot --package com.example.myapp',
  ],
  async run(args, context) {
    const flags = parseCommandArgs('screenshot', args, OPTIONS);
    const reporter = new Reporter({ verbose: flags.verbose ?? false });
    const port = parsePort(flags.port, '--port', 'screenshot');
    const device = flags.device === undefined ? null : deviceAddress(flags.device, 'screenshot');
    const path = flags.out === undefined ? defaultPath(device) : resolve(context.cwd, flags.out);
    const app = targetPackage(
      flags.package,
      resolve(context.cwd, flags.project ?? '.'),
      'screenshot',
    );
    await reporter.step(
      'Screenshot',
      async () => {
        const frame = await captureScreen(port, device, app);
        await mkdir(dirname(path), { recursive: true });
        await writeFile(path, encodePng(frame));
        return path;
      },
      (saved) => (app === null ? saved : `${app}, ${saved}`),
    );
  },
};
