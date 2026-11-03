import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';

interface FakeVoice {
  isAvailable(): boolean;
  setScreen(screen: string | null): void;
  addListener(event: string): void;
  removeListener(event: string): void;
}

const state = vi.hoisted(() => {
  const listeners = new Map<string, (payload: unknown) => void>();
  const shared: { native: FakeVoice | null; listeners: typeof listeners } = {
    native: null,
    listeners,
  };
  return shared;
});

vi.mock('react-native', () => ({
  TurboModuleRegistry: { get: () => state.native },
  DeviceEventEmitter: {
    addListener: (event: string, listener: (payload: unknown) => void) => {
      state.listeners.set(event, listener);
      return { remove: () => state.listeners.delete(event) };
    },
  },
}));

async function loadVoice() {
  vi.resetModules();
  return (await import('./voice.ts')).Voice;
}

beforeEach(() => {
  state.native = {
    isAvailable: vi.fn(() => true),
    setScreen: vi.fn(),
    addListener: vi.fn(),
    removeListener: vi.fn(),
  };
});

afterEach(() => {
  state.listeners.clear();
});

describe('Voice', () => {
  it('hands a listener every search the TV sends, and none it cannot read', async () => {
    const Voice = await loadVoice();
    const heard: unknown[] = [];

    Voice.addListener('search', (search) => heard.push(search));
    state.listeners.get('rnTizenVoiceSearch')?.({ utterance: 'Dune' });
    state.listeners.get('rnTizenVoiceSearch')?.({ nonsense: true });

    expect(heard).toEqual([{ utterance: 'Dune' }]);
    expect(state.native?.addListener).toHaveBeenCalledWith('search');
  });

  it('stops listening once, however often it is removed', async () => {
    const Voice = await loadVoice();
    const subscription = Voice.addListener('command', () => undefined);

    subscription.remove();
    subscription.remove();

    expect(state.listeners.has('rnTizenVoiceCommand')).toBe(false);
    expect(state.native?.removeListener).toHaveBeenCalledTimes(1);
  });

  it('passes the screen and availability through to the native module', async () => {
    const Voice = await loadVoice();

    Voice.setScreen('player');

    expect(Voice.isAvailable).toBe(true);
    expect(state.native?.setScreen).toHaveBeenCalledWith('player');
  });

  it('does nothing where there is no voice module', async () => {
    state.native = null;
    const Voice = await loadVoice();

    const subscription = Voice.addListener('search', () => undefined);
    subscription.remove();
    Voice.setScreen(null);

    expect(Voice.isAvailable).toBe(false);
    expect(state.listeners.size).toBe(0);
  });
});
