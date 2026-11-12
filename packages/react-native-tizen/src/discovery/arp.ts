import { run } from '../exec.ts';
import { prefixOf } from './lan.ts';

const ADDRESS = /\((\d{1,3}(?:\.\d{1,3}){3})\)/;
const MAX_OUTPUT = 256 * 1024;

/** The addresses `arp -an` lists, but the incomplete entries. */
export function parseArpTable(output: string): string[] {
  const hosts: string[] = [];
  for (const line of output.slice(0, MAX_OUTPUT).split('\n')) {
    const address = line.includes('incomplete') ? undefined : ADDRESS.exec(line)?.[1];
    if (address !== undefined && prefixOf(address) !== null && !hosts.includes(address))
      hosts.push(address);
  }
  return hosts;
}

/** The addresses this machine talked to lately, which a scan probes first. None when `arp` fails. */
export async function arpHosts(): Promise<string[]> {
  try {
    return parseArpTable(await run(['arp', '-an'], { quiet: true }));
  } catch {
    return [];
  }
}
