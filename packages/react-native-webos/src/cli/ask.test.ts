import { PassThrough } from 'node:stream';
import { describe, expect, it } from 'vitest';
import { ask, canAsk } from './ask.ts';

function answering(text: string): { input: PassThrough; output: PassThrough; shown: () => string } {
  const input = new PassThrough();
  const output = new PassThrough();
  let shown = '';
  output.on('data', (chunk: Buffer) => {
    shown += chunk.toString('utf8');
  });
  input.end(text);
  return { input, output, shown: () => shown };
}

describe('ask', () => {
  it('shows the default and takes the answer, or the default for an empty one', async () => {
    const given = answering('com.example.tv\n');
    const empty = answering('\n');

    expect(await ask('webOS package id', 'com.anonymous.app', given.input, given.output)).toBe(
      'com.example.tv',
    );
    expect(await ask('webOS package id', 'com.anonymous.app', empty.input, empty.output)).toBe(
      'com.anonymous.app',
    );
    expect(given.shown()).toContain('webOS package id (com.anonymous.app)');
  });
});

describe('ask without a default', () => {
  it('shows the question alone', async () => {
    const given = answering('A1b2C3\n');

    expect(await ask('Passphrase', '', given.input, given.output)).toBe('A1b2C3');
    expect(given.shown()).toBe('Passphrase ');
  });
});

describe('canAsk', () => {
  it('asks only when both ends are a terminal', () => {
    expect(canAsk({ isTTY: true }, { isTTY: true })).toBe(true);
    expect(canAsk({ isTTY: true }, {})).toBe(false);
    expect(canAsk({}, { isTTY: true })).toBe(false);
  });
});
