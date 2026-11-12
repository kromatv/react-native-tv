import type { Reporter } from '../cli/reporter.ts';
import type { TvProbe } from '../discovery/probe-tv.ts';
import { webosVersionName } from '../discovery/webos-version.ts';
import { addressText } from '../webos/device-address.ts';

function developerMode(tv: TvProbe): string {
  if (tv.developerMode) return 'on';
  return "off: install the Developer Mode app from LG's store on the TV and turn Dev Mode Status on";
}

function keyServer(tv: TvProbe): string {
  if (tv.keyServer) return 'on';
  return tv.connection.state === 'ready'
    ? 'off, and not needed while rn-webos is connected'
    : 'off: turn Key Server on in the Developer Mode app';
}

function connection(tv: TvProbe): string {
  const { connection: state } = tv;
  if (state.state === 'ready') return `connected as ${state.name}`;
  const address = addressText({ host: tv.host, port: tv.sshPort });
  if (state.state === 'unknown') return `not connected: rn-webos connect ${address}`;
  return `connected as ${state.name}, but ${state.reason}`;
}

/** Prints what one TV answered: its name and model, then a line per thing `rn-webos` needs of it. */
export function printTv(reporter: Reporter, tv: TvProbe): void {
  const title = [tv.name, tv.model].filter((part) => part !== '').join('  ');
  reporter.line(`${reporter.paint('bold', tv.host)}  ${title}`);
  if (tv.webosVersion !== null) reporter.field('webOS', webosVersionName(tv.webosVersion));
  reporter.field('Developer Mode', developerMode(tv));
  reporter.field('Key Server', keyServer(tv));
  reporter.field('rn-webos', connection(tv));
}
