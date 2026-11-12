import { describe, expect, it } from 'vitest';
import { UsageError } from './command.ts';
import { chooseLogEndpoint, parsePort } from './log-options.ts';

const choice = {
  command: 'package',
  release: undefined,
  host: undefined,
  port: undefined,
  deviceHost: null,
};

describe('parsePort', () => {
  it('defaults to 8099', () => {
    expect(parsePort(undefined, '--port', 'logs')).toBe(8099);
  });

  it('reads a port given as text', () => {
    expect(parsePort('9000', '--port', 'logs')).toBe(9000);
  });

  it('refuses something that is not a port', () => {
    expect(() => parsePort('99999', '--port', 'logs')).toThrow(UsageError);
    expect(() => parsePort('http', '--port', 'logs')).toThrow(
      '--port must be a TCP port, got http',
    );
  });
});

describe('chooseLogEndpoint', () => {
  it('streams no logs from a release build', () => {
    expect(chooseLogEndpoint({ ...choice, release: true })).toBeNull();
  });

  it('uses the host and port it is given', () => {
    expect(chooseLogEndpoint({ ...choice, host: '10.0.0.2', port: '9100' })).toEqual({
      host: '10.0.0.2',
      port: 9100,
    });
  });
});
