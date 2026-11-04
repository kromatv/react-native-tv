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

    expect(await ask('Tizen package id', 'com.anonymous.app', given.input, given.output)).toBe(
      'com.example.tv',
    );
    expect(await ask('Tizen package id', 'com.anonymous.app', empty.input, empty.output)).toBe(
      'com.anonymous.app',
    );
    expect(given.shown()).toContain('Tizen package id (com.anonymous.app)');
  });
});

describe('canAsk', () => {
  it('asks only when both ends are a terminal', () => {
    expect(canAsk({ isTTY: true }, { isTTY: true })).toBe(true);
    expect(canAsk({ isTTY: true }, {})).toBe(false);
    expect(canAsk({}, { isTTY: true })).toBe(false);
  });
});
