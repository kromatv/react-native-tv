import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';

interface FakeVoice {
  isAvailable(): boolean;
  unavailableReason(): string | null;
  canSelectTitles(): boolean;
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
    unavailableReason: vi.fn((): string | null => null),
    canSelectTitles: vi.fn(() => true),
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
    expect(Voice.unavailableReason).toBeNull();
    expect(Voice.canSelectTitles).toBe(true);
    expect(state.native?.setScreen).toHaveBeenCalledWith('player');
  });

  it('says why voice is unavailable on an older TV', async () => {
    vi.mocked(state.native?.unavailableReason ?? vi.fn()).mockReturnValue(
      'Voice control needs Tizen 6.0, and this TV runs 5.5',
    );
    const Voice = await loadVoice();

    expect(Voice.unavailableReason).toBe('Voice control needs Tizen 6.0, and this TV runs 5.5');
  });

  it('does nothing where there is no voice module', async () => {
    state.native = null;
    const Voice = await loadVoice();

    const subscription = Voice.addListener('search', () => undefined);
    subscription.remove();
    Voice.setScreen(null);

    expect(Voice.isAvailable).toBe(false);
    expect(Voice.unavailableReason).toBe('voice control needs a Samsung TV');
    expect(Voice.canSelectTitles).toBe(false);
    expect(state.listeners.size).toBe(0);
  });
});
