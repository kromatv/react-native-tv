import { describe, expect, it } from 'vitest';
import { symbolizeCrashReport } from './native-crash.ts';

const REPORT = [
  'Native crash: SIGSEGV at 0x0',
  '#0 libreact-native-tizen.so+0x1a2b',
  '#1 libc.so.6+0x40',
  '#2 0xdeadbeef',
].join('\n');

describe('symbolizeCrashReport', () => {
  it('appends the function each library frame is in', () => {
    const named = symbolizeCrashReport(REPORT, (frames) =>
      frames.map((frame) =>
        frame.library === 'libreact-native-tizen.so' ? 'rntizen::boom() at Boom.cpp:3' : null,
      ),
    );

    expect(named.split('\n')).toEqual([
      'Native crash: SIGSEGV at 0x0',
      '#0 libreact-native-tizen.so+0x1a2b  rntizen::boom() at Boom.cpp:3',
      '#1 libc.so.6+0x40',
      '#2 0xdeadbeef',
    ]);
  });

  it('hands the resolver the library and offset of every frame', () => {
    const seen: string[] = [];
    symbolizeCrashReport(REPORT, (frames) => {
      for (const frame of frames) seen.push(`${frame.library}@${frame.offset}`);
      return frames.map(() => null);
    });

    expect(seen).toEqual(['libreact-native-tizen.so@1a2b', 'libc.so.6@40']);
  });

  it('leaves a message without frames untouched', () => {
    expect(symbolizeCrashReport('Running "main"', () => [])).toBe('Running "main"');
  });
});
