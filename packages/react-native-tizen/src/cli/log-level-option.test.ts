import { describe, expect, it } from 'vitest';
import { parseLevel, untilInterrupted } from './log-level-option.ts';

describe('parseLevel', () => {
  it('defaults to info and refuses a level it does not know', () => {
    expect(parseLevel(undefined, 'logs')).toBe('info');
    expect(parseLevel('debug', 'logs')).toBe('debug');
    expect(() => parseLevel('loud', 'logs')).toThrow('--level must be one of');
  });
});

describe('untilInterrupted', () => {
  it('resolves at Ctrl+C', async () => {
    const interrupted = untilInterrupted();

    process.emit('SIGINT', 'SIGINT');

    await expect(interrupted).resolves.toBeUndefined();
  });
});
