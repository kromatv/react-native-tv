import { describe, expect, it } from 'vitest';
import { painter } from '../paint.ts';
import { formatLogEntry, isAtLeast, parseLogLine, printable } from './log-line.ts';

const NINE_AM = new Date(2026, 10, 2, 9, 5, 3);

describe('parseLogLine', () => {
  it('reads the level and message the host streams', () => {
    expect(parseLogLine('{"message":"Running \\"main\\"","level":"info"}')).toEqual({
      level: 'info',
      message: 'Running "main"',
    });
  });

  it('keeps a line that is not JSON as an info message', () => {
    expect(parseLogLine('Segmentation fault')).toEqual({
      level: 'info',
      message: 'Segmentation fault',
    });
  });

  it('treats an unknown level as info', () => {
    expect(parseLogLine('{"level":"fatal","message":"x"}').level).toBe('info');
  });

  it('strips the control characters of a JSON message', () => {
    expect(parseLogLine('{"level":"info","message":"\\u001b]0;owned\\u0007hi"}').message).toBe(
      ']0;ownedhi',
    );
  });

  it('strips the control characters of a raw line', () => {
    expect(parseLogLine('crash\u001b[31m here\u009b').message).toBe('crash[31m here');
  });

  it('keeps JSON without a message as raw text', () => {
    expect(parseLogLine('{"level":"warn"}')).toEqual({
      level: 'info',
      message: '{"level":"warn"}',
    });
  });
});

describe('formatLogEntry', () => {
  const plain = painter(false);

  it('prints the time, the level and the message on one line', () => {
    expect(formatLogEntry({ level: 'warn', message: 'Slow frame' }, NINE_AM, plain)).toBe(
      '09:05:03 warn  Slow frame',
    );
  });

  it('indents the continuation lines of a stack trace under the message', () => {
    const entry = { level: 'error' as const, message: 'TypeError: x\n    at App (app.tsx:3)' };

    expect(formatLogEntry(entry, NINE_AM, plain)).toBe(
      '09:05:03 error TypeError: x\n                   at App (app.tsx:3)',
    );
  });

  it('colors the level when colors are on', () => {
    const line = formatLogEntry({ level: 'error', message: 'boom' }, NINE_AM, painter(true));

    expect(line).toContain('\u001b[31merror\u001b[39m');
  });
});

describe('isAtLeast', () => {
  it('lets a louder level through a quieter minimum', () => {
    expect(isAtLeast('error', 'info')).toBe(true);
    expect(isAtLeast('info', 'info')).toBe(true);
  });

  it('holds back a quieter level', () => {
    expect(isAtLeast('debug', 'info')).toBe(false);
  });
});

describe('printable', () => {
  it('keeps line feeds and tabs', () => {
    expect(printable('a\tb\nc')).toBe('a\tb\nc');
  });

  it('drops C0 controls, delete and C1 controls', () => {
    expect(printable('a\u0000\u0008\r\u001b\u007f\u0085\u009fb')).toBe('ab');
  });

  it('keeps text beyond the control ranges', () => {
    expect(printable('héllo wörld ✓')).toBe('héllo wörld ✓');
  });
});
