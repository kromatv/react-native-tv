import { describe, expect, it } from 'vitest';
import { parsePerfFlags } from './perf-options.ts';

describe('parsePerfFlags', () => {
  it('runs until Ctrl+C, saves nothing and prints warnings by default', () => {
    expect(parsePerfFlags({}, 'perf', '/app')).toEqual({
      durationMs: null,
      out: null,
      minimum: 'warn',
    });
  });

  it('reads a duration in seconds and saves where asked, from the working directory', () => {
    expect(
      parsePerfFlags({ duration: '2.5', out: 'boot.jsonl', level: 'error' }, 'perf', '/app'),
    ).toEqual({
      durationMs: 2500,
      out: '/app/boot.jsonl',
      minimum: 'error',
    });
  });

  it('refuses a duration that is not a positive number', () => {
    expect(() => parsePerfFlags({ duration: '0' }, 'perf', '/app')).toThrow(
      '--duration must be a number of seconds, got 0',
    );
  });
});
