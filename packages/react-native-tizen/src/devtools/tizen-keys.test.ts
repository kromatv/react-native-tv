import { describe, expect, it, vi } from 'vitest';
import { expoReadsKeys, isExpoKeyHandler } from './expo-key-input.ts';
import { FakeStdin, runNow } from './expo-stdin.fixtures.ts';
import { listenForTizenKeys, type TizenKeyActions } from './tizen-keys.ts';

function actions(overrides: Partial<TizenKeyActions> = {}): TizenKeyActions {
  return {
    open: vi.fn(async () => undefined),
    choose: vi.fn(async () => undefined),
    usage: vi.fn(),
    busy: vi.fn(),
    defer: runNow,
    ...overrides,
  };
}

function started(given = actions()) {
  const stdin = new FakeStdin();
  listenForTizenKeys(stdin, given);
  stdin.startExpo();
  return { stdin, given };
}

describe('expoReadsKeys', () => {
  it("knows Expo's handler by name, and only while stdin is raw", () => {
    const stdin = new FakeStdin();

    expect(expoReadsKeys(stdin)).toBe(false);
    stdin.startExpo();
    expect(expoReadsKeys(stdin)).toBe(true);
    stdin.pauseExpo();
    expect(expoReadsKeys(stdin)).toBe(false);
    expect(isExpoKeyHandler(() => undefined)).toBe(false);
  });
});

describe('listenForTizenKeys', () => {
  it('prints the usage line once Expo reads keys, and waits for Expo until then', () => {
    const stdin = new FakeStdin();
    const given = actions();
    listenForTizenKeys(stdin, given);

    stdin.press('t');
    expect(given.usage).not.toHaveBeenCalled();

    stdin.startExpo();
    stdin.pauseExpo();
    stdin.startExpo();

    expect(given.usage).toHaveBeenCalledOnce();
    expect(given.usage).toHaveBeenCalledWith(false);
  });

  it('opens on t, asks on T, and adds its lines to the list ? prints', () => {
    const { stdin, given } = started();

    stdin.press('t');
    stdin.press('?');

    expect(given.open).toHaveBeenCalledOnce();
    expect(given.usage).toHaveBeenLastCalledWith(true);
    expect(stdin.expoKeys).toEqual(['t', '?']);
  });

  it('asks which target on T', async () => {
    const { stdin, given } = started();

    stdin.press('T');
    await Promise.resolve();

    expect(given.choose).toHaveBeenCalledOnce();
    expect(given.open).not.toHaveBeenCalled();
  });

  it("does nothing for a key typed into one of Expo's prompts", () => {
    const { stdin, given } = started();

    stdin.pauseExpo();
    stdin.press('t');
    stdin.press('T');
    stdin.press('?');

    expect(given.open).not.toHaveBeenCalled();
    expect(given.choose).not.toHaveBeenCalled();
    expect(given.usage).toHaveBeenCalledOnce();
  });

  it('ignores other keys and pasted text', () => {
    const { stdin, given } = started();

    stdin.press('a');
    stdin.press('tt');

    expect(given.open).not.toHaveBeenCalled();
  });

  it('says it is busy rather than opening twice, and takes t again once done', async () => {
    let finish: () => void = () => undefined;
    const open = vi.fn(
      () =>
        new Promise<void>((resolve) => {
          finish = resolve;
        }),
    );
    const { stdin, given } = started(actions({ open }));

    stdin.press('t');
    stdin.press('T');
    finish();
    await new Promise((resolve) => setImmediate(resolve));
    stdin.press('t');

    expect(given.busy).toHaveBeenCalledOnce();
    expect(open).toHaveBeenCalledTimes(2);
  });
});
