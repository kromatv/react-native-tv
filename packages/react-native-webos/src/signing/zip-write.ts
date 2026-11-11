import { crc32, deflateRawSync } from 'node:zlib';
import type { ZipEntry } from './zip-read.ts';

const UTF8_NAMES = 0x0800;
const VERSION = 20;
const DIRECTORY_ATTRIBUTE = 0x10;

function dosTime(date: Date): { readonly time: number; readonly day: number } {
  return {
    time: (date.getHours() << 11) | (date.getMinutes() << 5) | Math.floor(date.getSeconds() / 2),
    day: ((date.getFullYear() - 1980) << 9) | ((date.getMonth() + 1) << 5) | date.getDate(),
  };
}

/** Writes entries as a zip archive in the order given: files deflated, directories stored. */
export function writeZip(entries: readonly ZipEntry[], modified = new Date()): Buffer {
  const { time, day } = dosTime(modified);
  const locals: Buffer[] = [];
  const centrals: Buffer[] = [];
  let offset = 0;
  for (const entry of entries) {
    const name = Buffer.from(entry.directory ? `${entry.name}/` : entry.name, 'utf8');
    const data = entry.directory ? Buffer.alloc(0) : deflateRawSync(entry.data);
    const flags = name.some((byte) => byte > 0x7f) ? UTF8_NAMES : 0;
    const method = entry.directory ? 0 : 8;
    const checksum = entry.directory ? 0 : crc32(entry.data);
    const local = Buffer.alloc(30);
    local.writeUInt32LE(0x04034b50, 0);
    local.writeUInt16LE(VERSION, 4);
    local.writeUInt16LE(flags, 6);
    local.writeUInt16LE(method, 8);
    local.writeUInt16LE(time, 10);
    local.writeUInt16LE(day, 12);
    local.writeUInt32LE(checksum, 14);
    local.writeUInt32LE(data.length, 18);
    local.writeUInt32LE(entry.data.length, 22);
    local.writeUInt16LE(name.length, 26);
    const central = Buffer.alloc(46);
    central.writeUInt32LE(0x02014b50, 0);
    central.writeUInt16LE(VERSION, 4);
    local.copy(central, 6, 4, 30);
    central.writeUInt32LE(entry.directory ? DIRECTORY_ATTRIBUTE : 0, 38);
    central.writeUInt32LE(offset, 42);
    locals.push(local, name, data);
    centrals.push(central, name);
    offset += local.length + name.length + data.length;
  }
  const directory = Buffer.concat(centrals);
  const end = Buffer.alloc(22);
  end.writeUInt32LE(0x06054b50, 0);
  end.writeUInt16LE(entries.length, 8);
  end.writeUInt16LE(entries.length, 10);
  end.writeUInt32LE(directory.length, 12);
  end.writeUInt32LE(offset, 16);
  return Buffer.concat([...locals, directory, end]);
}
