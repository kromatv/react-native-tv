import { scanForTvs, type FoundTv } from '../discovery/scan-tvs.ts';
import type { SdbAddress } from '../sdb/sdb-connection.ts';
import { deviceSerial } from '../tizen/device-address.ts';
import { readKnownDevices } from '../tizen/known-devices.ts';
import { CommandInputError } from './devtools-commands.ts';

/** Where the app opens: `simulator`, or a TV's `host:port`. */
export const SIMULATOR = 'simulator';

/** One entry of the list `shift+t` shows. */
export interface TizenTarget {
  readonly target: string;
  readonly label: string;
}

/** How TVs are found, replaced in tests. */
export interface TvFinder {
  readonly knownDevices: () => Promise<readonly SdbAddress[]>;
  readonly scan: () => Promise<readonly FoundTv[]>;
  readonly searching: () => void;
}

const defaultFinder: TvFinder = {
  knownDevices: readKnownDevices,
  scan: () => scanForTvs(),
  searching: () => undefined,
};

function inDeveloperMode(found: readonly FoundTv[]): FoundTv[] {
  return found.filter((tv) => tv.developerMode !== false);
}

/**
 * The TV `t` opens on: the one used last, else the only TV in Developer mode
 * on the network, else an error that says what to do.
 */
export async function lastOrOnlyTv(overrides: Partial<TvFinder> = {}): Promise<string> {
  const finder = { ...defaultFinder, ...overrides };
  const [last] = await finder.knownDevices();
  if (last !== undefined) return deviceSerial(last);
  finder.searching();
  const usable = inDeveloperMode(await finder.scan());
  const [tv] = usable;
  if (usable.length === 1 && tv !== undefined) {
    return deviceSerial({ host: tv.host, port: tv.sdbPort });
  }
  if (usable.length === 0) {
    throw new CommandInputError(
      'No TV in Developer mode answered on this network: on the TV, open Apps and type 12345 on the remote, or press shift+t for the simulator.',
    );
  }
  const hosts = usable.map((each) => each.host).join(', ');
  throw new CommandInputError(
    `${usable.length} TVs answered (${hosts}): press shift+t to pick one.`,
  );
}

/** What `shift+t` offers: the TVs used before, those on the network, then the simulator. */
export async function tizenTargets(overrides: Partial<TvFinder> = {}): Promise<TizenTarget[]> {
  const finder = { ...defaultFinder, ...overrides };
  const known = await finder.knownDevices();
  finder.searching();
  const found = inDeveloperMode(await finder.scan());
  const named = new Map(
    found.map((tv) => [tv.host, [tv.name, tv.model].filter(Boolean).join(' ')]),
  );
  const seen = new Set<string>();
  const tvs: TizenTarget[] = [];
  const add = (address: SdbAddress, note: string) => {
    if (seen.has(address.host)) return;
    seen.add(address.host);
    const name = named.get(address.host) ?? '';
    tvs.push({
      target: deviceSerial(address),
      label: [address.host, name, note].filter(Boolean).join('  '),
    });
  };
  known.forEach((address, index) => add(address, index === 0 ? '(used last)' : ''));
  for (const tv of found) add({ host: tv.host, port: tv.sdbPort }, '');
  return [...tvs, { target: SIMULATOR, label: 'Tizen simulator' }];
}
