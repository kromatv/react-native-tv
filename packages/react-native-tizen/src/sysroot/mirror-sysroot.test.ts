import {
  lstatSync,
  mkdirSync,
  mkdtempSync,
  readFileSync,
  readlinkSync,
  rmSync,
  symlinkSync,
  writeFileSync,
} from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { BINDING, bindings, sharedObject } from './elf.fixtures.ts';
import { mirrorSysroot } from './mirror-sysroot.ts';

const { LOCAL, GLOBAL, WEAK } = BINDING;
const LIBC_SCRIPT = 'GROUP ( /usr/lib/libc.so.6 /usr/lib/libc_nonshared.a )\n';

let dir: string;
let rootstrap: string;
let destination: string;
let compatArchive: string;

function file(relative: string, contents: string | Uint8Array): void {
  const path = join(rootstrap, relative);
  mkdirSync(join(path, '..'), { recursive: true });
  writeFileSync(path, contents);
}

function mirror() {
  return mirrorSysroot({ rootstrap, destination, compatArchive });
}

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tizen-sysroot-'));
  rootstrap = join(dir, 'rootstrap');
  destination = join(dir, 'sysroots', 'tizen-9.0-device.core');
  compatArchive = join(dir, 'compat.a');
  writeFileSync(compatArchive, '!<arch>\n');
  file('usr/include/stdio.h', '');
  file('usr/lib/libstub.so', sharedObject(false, [LOCAL, GLOBAL, LOCAL], 1));
  file('usr/lib/libclean.so', sharedObject(false, [LOCAL, GLOBAL], 1));
  file('usr/lib/libc.so', LIBC_SCRIPT);
  file('usr/lib/crt1.o', '');
  mkdirSync(join(rootstrap, 'usr', 'lib', 'pkgconfig'));
  symlinkSync('libclean.so', join(rootstrap, 'usr', 'lib', 'libclean.so.1'));
  file('etc/os-release', '');
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
});

describe('mirrorSysroot', () => {
  it('copies only the libraries it patches and links everything else', async () => {
    const mirrored = await mirror();

    expect(mirrored).toEqual({ path: destination, patchedLibraries: 1, reused: false });
    const stub = new Uint8Array(readFileSync(join(destination, 'usr/lib/libstub.so')));
    expect(bindings(stub, false, 3)).toEqual([LOCAL, GLOBAL, WEAK]);
    expect(lstatSync(join(destination, 'usr/lib/libclean.so')).isSymbolicLink()).toBe(true);
    expect(readlinkSync(join(destination, 'usr/lib/libclean.so.1'))).toBe('libclean.so');
    expect(lstatSync(join(destination, 'usr/lib/pkgconfig')).isSymbolicLink()).toBe(true);
    expect(lstatSync(join(destination, 'usr/include')).isSymbolicLink()).toBe(true);
    expect(lstatSync(join(destination, 'etc')).isSymbolicLink()).toBe(true);
  });

  it("adds the compat archive to libc.so's linker script, beside it", async () => {
    await mirror();

    expect(readFileSync(join(destination, 'usr/lib/libc.so'), 'utf8')).toBe(
      'GROUP ( /usr/lib/libc.so.6 /usr/lib/libc_nonshared.a /usr/lib/librn-tizen-glibc-compat.a )\n',
    );
    expect(readFileSync(join(destination, 'usr/lib/librn-tizen-glibc-compat.a'), 'utf8')).toBe(
      '!<arch>\n',
    );
  });

  it('reuses a mirror of the same rootstrap and compat archive', async () => {
    await mirror();

    expect((await mirror()).reused).toBe(true);
  });

  it('mirrors again when the compat archive changed', async () => {
    await mirror();
    writeFileSync(compatArchive, '!<arch>\nchanged');

    expect((await mirror()).reused).toBe(false);
  });
});
