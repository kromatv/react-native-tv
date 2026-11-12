import { chmodSync, mkdirSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';

const ELF_MACHINE_OFFSET = 18;

/** The first bytes of a little-endian 32-bit ELF executable for `machine`. */
export function elfHeader(machine: number): Buffer {
  const header = Buffer.alloc(128);
  header.set([0x7f, 0x45, 0x4c, 0x46, 1, 1, 1]);
  header.writeUInt16LE(2, 16);
  header.writeUInt16LE(machine, ELF_MACHINE_OFFSET);
  return header;
}

/**
 * Writes a small native app into `dir`, laid out as a package's app directory:
 * `appinfo.json`, the executable it names, an icon, a resource directory and
 * an empty `appdata`.
 */
export function writeProbeApp(dir: string, appInfo: Record<string, unknown> = {}): void {
  mkdirSync(join(dir, 'res', 'assets'), { recursive: true });
  mkdirSync(join(dir, 'appdata'));
  chmodSync(join(dir, 'appdata'), 0o777);
  writeFileSync(
    join(dir, 'appinfo.json'),
    JSON.stringify({
      id: 'com.example.probe',
      version: '1.2.3',
      type: 'native',
      main: 'react-native-webos',
      title: 'Probe',
      icon: 'icon.png',
      ...appInfo,
    }),
  );
  writeFileSync(join(dir, 'react-native-webos'), elfHeader(40));
  chmodSync(join(dir, 'react-native-webos'), 0o755);
  writeFileSync(join(dir, 'icon.png'), 'png');
  writeFileSync(join(dir, 'res', 'index.webos.hbc'), 'bytecode');
  writeFileSync(join(dir, 'res', 'assets', 'a.png'), 'asset');
}
