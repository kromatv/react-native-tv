import { describe, expect, it } from 'vitest';
import { painter } from '../paint.ts';
import type { Command } from './command.ts';
import { commandHelp, programHelp } from './help.ts';

const command: Command = {
  name: 'logs',
  summary: 'Stream the logs of a development build',
  options: {
    port: { type: 'string', placeholder: '<port>', description: 'Where to listen' },
    verbose: { type: 'boolean', short: 'v', description: 'Say more' },
  },
  examples: ['rn-tizen logs --port 9000'],
  run: () => Promise.resolve(),
};

describe('commandHelp', () => {
  it('lists every option with its value, short form and description, aligned', () => {
    expect(commandHelp(command, painter(false))).toBe(
      [
        'Usage: rn-tizen logs [options]',
        '',
        'Stream the logs of a development build',
        '',
        'Options',
        '  --port <port>   Where to listen',
        '  --verbose, -v   Say more',
        '',
        'Examples',
        '  rn-tizen logs --port 9000',
      ].join('\n'),
    );
  });
});

describe('programHelp', () => {
  it('lists every command with its summary', () => {
    expect(programHelp([command], '1.2.3', painter(false))).toContain(
      '  logs   Stream the logs of a development build',
    );
  });
});
