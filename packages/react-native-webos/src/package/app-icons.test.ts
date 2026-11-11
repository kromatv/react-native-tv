import { mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { FixableError } from '../errors.ts';
import { writeAppIcons, writeSplash } from './app-icons.ts';
import { decodePng, pngSize } from './png-image.ts';
import { plainPng } from './png.fixtures.ts';

let root: string;
let out: string;

beforeEach(() => {
  root = mkdtempSync(join(tmpdir(), 'rn-webos-icons-'));
  out = mkdtempSync(join(tmpdir(), 'rn-webos-icons-out-'));
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
  rmSync(out, { recursive: true, force: true });
});

const written = (name: string) => readFileSync(join(out, name));

describe('writeAppIcons', () => {
  it("scales the app's icon to the two sizes LG asks for", async () => {
    writeFileSync(join(root, 'icon.png'), plainPng(512, 512, [200, 10, 10, 255]));

    await writeAppIcons(fakeApp(root, { icon: './icon.png' }), out);

    expect(pngSize(written('icon.png'))).toEqual({ width: 80, height: 80 });
    expect(pngSize(written('largeIcon.png'))).toEqual({ width: 130, height: 130 });
    expect([...decodePng(written('icon.png')).pixels.subarray(0, 4)]).toEqual([200, 10, 10, 255]);
  });

  it('writes plain icons for an app without one', async () => {
    await writeAppIcons(fakeApp(root), out);

    expect(pngSize(written('icon.png'))).toEqual({ width: 80, height: 80 });
    expect(pngSize(written('largeIcon.png'))).toEqual({ width: 130, height: 130 });
    expect(decodePng(written('icon.png')).pixels[3]).toBe(255);
  });

  it('refuses an icon that does not exist or is not a PNG, naming where to fix it', async () => {
    writeFileSync(join(root, 'photo.png'), 'jpeg bytes under a png name, long enough to read');

    const missing = writeAppIcons(fakeApp(root, { icon: './missing.png' }), out);
    const notPng = writeAppIcons(fakeApp(root, { icon: './photo.png' }), out);

    await expect(missing).rejects.toThrow('The app icon ./missing.png does not exist');
    await expect(notPng).rejects.toBeInstanceOf(FixableError);
    await expect(notPng).rejects.toThrow('The app icon ./photo.png cannot be read as a PNG');
  });
});

describe('writeSplash', () => {
  it('copies a 1920x1080 splash image as it is', async () => {
    const splash = plainPng(1920, 1080);
    writeFileSync(join(root, 'splash.png'), splash);

    expect(await writeSplash(fakeApp(root, { splash: './splash.png' }), out)).toBe(true);
    expect(written('splash.png').equals(splash)).toBe(true);
  });

  it('writes nothing for an app without one', async () => {
    expect(await writeSplash(fakeApp(root), out)).toBe(false);
  });

  it('refuses another size, saying which one the TV takes', async () => {
    writeFileSync(join(root, 'splash.png'), plainPng(1280, 720));

    await expect(writeSplash(fakeApp(root, { splash: './splash.png' }), out)).rejects.toThrow(
      'is 1280x720, and a webOS TV takes 1920x1080',
    );
  });
});
