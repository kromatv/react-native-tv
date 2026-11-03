import { PassThrough } from 'node:stream';
import { describe, expect, it } from 'vitest';
import { painter } from '../paint.ts';
import { PerfListener } from './perf-listener.ts';
import { idleSample } from './perf-sample.fixtures.ts';

const NINE_AM = new Date(2026, 10, 3, 9, 0, 5);

function listenerFor(device: string | null) {
  const out = new PassThrough();
  const listener = new PerfListener({
    paint: painter(false),
    minimum: 'warn',
    device,
    out,
    now: () => NINE_AM,
  });
  const printed = () => String(out.read() ?? '');
  return { listener, printed };
}

const perfEntry = (frames: number) => ({
  level: 'debug' as const,
  message: `perf ${JSON.stringify(idleSample({ frames }))}`,
});

describe('PerfListener', () => {
  it('asks every app for perf samples', () => {
    expect(listenerFor(null).listener.wantsPerf).toBe(true);
  });

  it('keeps and prints each sample', () => {
    const { listener, printed } = listenerFor(null);

    listener.onEntry(perfEntry(58), '192.168.1.50');

    expect(listener.samples.map((sample) => sample.frames)).toEqual([58]);
    expect(printed()).toContain('fps  58/60');
  });

  it('prints log lines at the minimum level or louder, and drops quieter ones', () => {
    const { listener, printed } = listenerFor(null);

    listener.onEntry({ level: 'info', message: 'Running "main"' }, '192.168.1.50');
    listener.onEntry({ level: 'warn', message: 'Slow frame' }, '192.168.1.50');

    expect(printed()).toBe('09:00:05 warn  Slow frame\n');
  });

  it('ignores another TV when one was named', () => {
    const { listener, printed } = listenerFor('192.168.1.50');

    listener.onEntry(perfEntry(60), '192.168.1.200');
    listener.onConnect('192.168.1.200');

    expect(listener.samples).toEqual([]);
    expect(printed()).toBe('');
  });
});

describe('PerfListener.firstSample', () => {
  it('settles once a sample arrives', async () => {
    const { listener } = listenerFor(null);

    listener.onEntry({ level: 'warn', message: 'not yet' }, '192.168.1.50');
    listener.onEntry(perfEntry(1), '192.168.1.50');

    await expect(listener.firstSample).resolves.toBeUndefined();
  });
});
