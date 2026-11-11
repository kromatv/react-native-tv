import { scanForTvs, type FoundTv } from '../discovery/scan-tvs.ts';
import { addressText } from '../webos/device-address.ts';
import { readKnownDevices, type KnownDevice } from '../webos/known-devices.ts';
import { CommandInputError } from './devtools-commands.ts';

/** Where the app opens: `simulator`, or what `--device` takes for a TV. */
export const SIMULATOR = 'simulator';

/** One entry of the list `shift+t` shows. */
export interface WebOSTarget {
  readonly target: string;
  readonly label: string;
}

/** How TVs are found, replaced in tests. */
export interface TvFinder {
  readonly knownDevices: () => Promise<readonly KnownDevice[]>;
  readonly scan: () => Promise<readonly FoundTv[]>;
  readonly searching: () => void;
}

const defaultFinder: TvFinder = {
  knownDevices: readKnownDevices,
  scan: () => scanForTvs(),
  searching: () => undefined,
};

function inDeveloperMode(found: readonly FoundTv[]): FoundTv[] {
  return found.filter((tv) => tv.developerMode);
}

/**
 * The TV `t` opens on: the one connected last, else an error that says what
 * to do, naming the TVs in Developer Mode the network shows.
 */
export async function lastOrOnlyTv(overrides: Partial<TvFinder> = {}): Promise<string> {
  const finder = { ...defaultFinder, ...overrides };
  const [last] = await finder.knownDevices();
  if (last !== undefined) return addressText(last);
  finder.searching();
  const usable = inDeveloperMode(await finder.scan());
  if (usable.length === 0) {
    throw new CommandInputError(
      "No TV in Developer Mode answered on this network: turn Dev Mode Status on in the TV's Developer Mode app, or press shift+t for the simulator.",
    );
  }
  const hosts = usable.map((each) => each.host).join(', ');
  throw new CommandInputError(
    `rn-webos is not connected to a TV yet. In Developer Mode on this network: ${hosts}. Turn Key Server on there and run: rn-webos connect <tv ip>`,
  );
}

/** What `shift+t` offers: the TVs connected before, the last one used first, then the simulator. */
export async function webosTargets(overrides: Partial<TvFinder> = {}): Promise<WebOSTarget[]> {
  const finder = { ...defaultFinder, ...overrides };
  const tvs = (await finder.knownDevices()).map((tv, index) => ({
    target: addressText(tv),
    label: [tv.host, tv.name, index === 0 ? '(used last)' : ''].filter(Boolean).join('  '),
  }));
  return [...tvs, { target: SIMULATOR, label: 'webOS simulator' }];
}
