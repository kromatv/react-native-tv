import { describe, expect, it } from 'vitest';
import { capturedOutput } from '../fakes.fixtures.ts';
import { painter } from '../paint.ts';
import { LogPrinter } from './log-printer.ts';

const paint = painter(false);

describe('LogPrinter', () => {
  it('prints entries at the minimum level and above', () => {
    const out = capturedOutput();
    const printer = new LogPrinter({ paint, minimum: 'warn', held: false, out });

    printer.onEntry({ level: 'info', message: 'hidden' });
    printer.onEntry({ level: 'error', message: 'shown' });

    expect(out.text()).not.toContain('hidden');
    expect(out.text()).toContain('shown');
  });

  it('holds what arrives until it is released', () => {
    const out = capturedOutput();
    const printer = new LogPrinter({ paint, minimum: 'debug', held: true, out });
    printer.onConnect('192.168.1.50');
    printer.onEntry({ level: 'info', message: 'early' });

    expect(out.text()).toBe('');
    printer.release();
    printer.onDisconnect('192.168.1.50');

    expect(out.text().split('\n')).toEqual([
      'The app connected from 192.168.1.50',
      expect.stringContaining('early'),
      'The app at 192.168.1.50 disconnected',
      '',
    ]);
  });

  it('keeps only the newest lines it held', () => {
    const out = capturedOutput();
    const printer = new LogPrinter({ paint, minimum: 'debug', held: true, out });
    for (let index = 0; index < 2001; index += 1) {
      printer.onEntry({ level: 'info', message: `line ${index}` });
    }

    printer.release();

    expect(out.text()).not.toContain('line 0\n');
    expect(out.text()).toContain('line 2000');
  });

  it('prints an entry as its transform rewrites it', () => {
    const out = capturedOutput();
    const printer = new LogPrinter({
      paint,
      minimum: 'debug',
      held: false,
      out,
      transform: (entry) => ({ ...entry, message: entry.message.toUpperCase() }),
    });

    printer.onEntry({ level: 'info', message: 'quiet' });

    expect(out.text()).toContain('QUIET');
  });
});
