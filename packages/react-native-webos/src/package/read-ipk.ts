import { gunzipSync } from 'node:zlib';
import { readAr, type ArEntry } from './ar-archive.ts';
import { parseControl } from './ipk-control.ts';
import { readTar, type TarRecord } from './ustar.ts';

/** A webOS package taken apart. */
export interface Ipk {
  /** The `ar` members, in the archive's order. */
  readonly members: readonly ArEntry[];
  /** The fields of the `control` file. */
  readonly control: ReadonlyMap<string, string>;
  readonly controlEntries: readonly TarRecord[];
  /** What the package installs. */
  readonly dataEntries: readonly TarRecord[];
}

const MAX_TAR_BYTES = 1024 * 1024 * 1024;

function tarMember(members: readonly ArEntry[], name: string): TarRecord[] {
  const member = members.find((candidate) => candidate.name.replace(/\/$/, '') === name);
  if (member === undefined) throw new Error(`The package holds no ${name}`);
  return readTar(gunzipSync(member.data, { maxOutputLength: MAX_TAR_BYTES }));
}

/** Reads an `.ipk`: its `ar` members, its `control` fields and the entries of both tar archives. */
export function readIpk(ipk: Uint8Array): Ipk {
  const members = readAr(ipk);
  const controlEntries = tarMember(members, 'control.tar.gz');
  const control = controlEntries.find((entry) => entry.path.replace(/^\.\//, '') === 'control');
  if (control === undefined) throw new Error('The package holds no control file');
  return {
    members,
    control: parseControl(Buffer.from(control.data).toString('utf8')),
    controlEntries,
    dataEntries: tarMember(members, 'data.tar.gz'),
  };
}
