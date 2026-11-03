import { networkInterfaces } from 'node:os';
import { z } from 'zod';
import { FixableError } from '../errors.ts';
import { pickLanAddress } from '../logs/lan-address.ts';
import { DEFAULT_LOG_PORT, type LogEndpoint } from '../package/runtime-config.ts';
import { UsageError, type OptionSpecs } from './command.ts';

/** `--release`, for a command that builds for development unless asked. */
export const RELEASE_OPTION = {
  release: {
    type: 'boolean',
    description: 'Build for the store: the app streams no logs',
  },
} as const satisfies OptionSpecs;

/** `--dev`, for a command that builds for the store unless asked. */
export const DEV_OPTION = {
  dev: {
    type: 'boolean',
    description: 'Build for development: the app streams its logs to this computer',
  },
} as const satisfies OptionSpecs;

/** The options that decide where a development build streams its logs. */
export const LOG_OPTIONS = {
  'log-host': {
    type: 'string',
    placeholder: '<ip>',
    description: "Where the app streams its logs (default: this computer's LAN address)",
  },
  'log-port': {
    type: 'string',
    placeholder: '<port>',
    description: `The port it streams them to (default: ${DEFAULT_LOG_PORT})`,
  },
} as const satisfies OptionSpecs;

const portSchema = z.coerce.number().int().min(1).max(65535);

/** Reads a port flag, or `fallback` when it is absent. */
export function parsePort(
  value: string | undefined,
  flag: string,
  command: string,
  fallback = DEFAULT_LOG_PORT,
): number {
  if (value === undefined) return fallback;
  const parsed = portSchema.safeParse(value);
  if (parsed.success) return parsed.data;
  throw new UsageError(`${flag} must be a TCP port, got ${value}`, command);
}

interface LogChoice {
  readonly command: string;
  readonly release: boolean | undefined;
  readonly host: string | undefined;
  readonly port: string | undefined;
  readonly deviceHost: string | null;
}

/** Where a build streams its logs, or null for a release build. */
export function chooseLogEndpoint(choice: LogChoice): LogEndpoint | null {
  if (choice.release === true) return null;
  const port = parsePort(choice.port, '--log-port', choice.command);
  const host = choice.host ?? pickLanAddress(networkInterfaces(), choice.deviceHost);
  if (host !== null) return { host, port };
  throw new FixableError(
    "Could not find this computer's address on the local network",
    'Pass --log-host <ip> with an address the TV can reach, or make a release build, which streams no logs.',
  );
}
