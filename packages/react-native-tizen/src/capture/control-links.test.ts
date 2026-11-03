import { Socket } from 'node:net';
import { afterEach, describe, expect, it, vi } from 'vitest';
import { ControlLink } from './control-link.ts';
import { ControlLinks } from './control-links.ts';

function link(address: string): ControlLink {
  return new ControlLink(new Socket(), address);
}

afterEach(() => {
  vi.useRealTimers();
});

describe('ControlLinks', () => {
  it('finds the newest link of the device asked for, or of the only TV', async () => {
    const links = new ControlLinks();
    const older = link('192.168.1.50');
    const newer = link('192.168.1.50');
    links.add(older);
    links.add(newer);

    expect(await links.find('192.168.1.50', 0)).toBe(newer);
    expect(await links.find(null, 0)).toBe(newer);
    links.remove(newer);
    links.remove(newer);
    expect(await links.find(null, 0)).toBe(older);
  });

  it('waits for a link to come, and gives up after its timeout', async () => {
    vi.useFakeTimers();
    const links = new ControlLinks();
    const arriving = link('192.168.1.50');

    const found = links.find('192.168.1.50', 1000);
    links.add(link('10.0.0.9'));
    links.add(arriving);
    const missing = links.find('10.0.0.1', 1000);
    await vi.advanceTimersByTimeAsync(1000);

    expect(await found).toBe(arriving);
    expect(await missing).toBeNull();
  });

  it('asks which TV when apps on two are connected', () => {
    const links = new ControlLinks();
    links.add(link('192.168.1.50'));
    links.add(link('192.168.1.51'));

    expect(() => links.find(null, 0)).toThrow(
      'Apps on 2 TVs are connected: 192.168.1.50, 192.168.1.51',
    );
  });

  it('keeps waiting while only other TVs connect', async () => {
    vi.useFakeTimers();
    const links = new ControlLinks();
    links.add(link('192.168.1.50'));

    const waiting = links.find('10.0.0.9', 1000);
    links.add(link('192.168.1.51'));
    const arriving = link('10.0.0.9');
    links.add(arriving);

    expect(await waiting).toBe(arriving);
  });
});
