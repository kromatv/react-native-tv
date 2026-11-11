import { existsSync } from 'node:fs';
import { copyFile, readFile, writeFile } from 'node:fs/promises';
import { join, resolve } from 'node:path';
import type { AppProject } from '../app/app-project.ts';
import { FixableError } from '../errors.ts';
import { ICON_FILE, LARGE_ICON_FILE, SPLASH_FILE } from './appinfo.ts';
import { encodePng } from './png-encode.ts';
import { decodePng, pngSize, type RgbaImage } from './png-image.ts';
import { fitImage } from './scale-image.ts';

const ICON_SIZE = 80;
const LARGE_ICON_SIZE = 130;
const SPLASH_WIDTH = 1920;
const SPLASH_HEIGHT = 1080;
const DEFAULT_ICON_COLOR = [38, 41, 48, 255] as const;
const ICON_HINT =
  'Fix expo.icon, or the icon prop of the @kromatv/react-native-webos plugin, in app.json.';
const SPLASH_HINT = 'Fix the splash prop of the @kromatv/react-native-webos plugin in app.json.';

async function readPng<Value>(
  app: AppProject,
  path: string,
  what: string,
  hint: string,
  read: (file: Buffer) => Value,
): Promise<Value> {
  const file = resolve(app.root, path);
  if (!existsSync(file)) throw new FixableError(`The ${what} ${path} does not exist`, hint);
  try {
    return read(await readFile(file));
  } catch (error) {
    const reason = error instanceof Error ? error.message : String(error);
    throw new FixableError(`The ${what} ${path} cannot be read as a PNG: ${reason}`, hint);
  }
}

function plainIcon(): RgbaImage {
  const pixels = new Uint8Array(LARGE_ICON_SIZE * LARGE_ICON_SIZE * 4);
  for (let at = 0; at < pixels.byteLength; at += 4) pixels.set(DEFAULT_ICON_COLOR, at);
  return { width: LARGE_ICON_SIZE, height: LARGE_ICON_SIZE, pixels };
}

/**
 * Writes the two icons `appinfo.json` names into `dir`, 80x80 and 130x130 as
 * LG asks, scaled from the app's icon, or plain when the app has none.
 */
export async function writeAppIcons(app: AppProject, dir: string): Promise<void> {
  const { icon } = app.config;
  const source =
    icon === undefined ? plainIcon() : await readPng(app, icon, 'app icon', ICON_HINT, decodePng);
  await writeFile(join(dir, ICON_FILE), encodePng(fitImage(source, ICON_SIZE, ICON_SIZE)));
  await writeFile(
    join(dir, LARGE_ICON_FILE),
    encodePng(fitImage(source, LARGE_ICON_SIZE, LARGE_ICON_SIZE)),
  );
}

/**
 * Copies the app's splash image into `dir` when it has one, and says whether
 * it did. LG shows it as it is, so it must be 1920x1080.
 */
export async function writeSplash(app: AppProject, dir: string): Promise<boolean> {
  const { splash } = app.config;
  if (splash === undefined) return false;
  const size = await readPng(app, splash, 'splash image', SPLASH_HINT, pngSize);
  if (size.width !== SPLASH_WIDTH || size.height !== SPLASH_HEIGHT) {
    throw new FixableError(
      `The splash image ${splash} is ${size.width}x${size.height}, and a webOS TV takes ${SPLASH_WIDTH}x${SPLASH_HEIGHT}`,
      SPLASH_HINT,
    );
  }
  await copyFile(resolve(app.root, splash), join(dir, SPLASH_FILE));
  return true;
}
