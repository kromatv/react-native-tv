import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';

interface FakeMicButton {
  isAvailable(): boolean;
  addListener(event: string): void;
  removeListener(event: string): void;
}

const state = vi.hoisted(() => {
  const listeners = new Set<(payload: unknown) => void>();
  const shared: {
    native: FakeMicButton | null;
    listeners: typeof listeners;
    event: string | null;
  } = {
    native: null,
    listeners,
    event: null,
  };
  return shared;
});

vi.mock('react-native', () => ({
  TurboModuleRegistry: { get: () => state.native },
  DeviceEventEmitter: {
    addListener: (event: string, listener: (payload: unknown) => void) => {
      state.event = event;
      state.listeners.add(listener);
      return { remove: () => state.listeners.delete(listener) };
    },
  },
}));

async function loadMicButton() {
  vi.resetModules();
  return (await import('./mic-button.ts')).MicButton;
}

function send(payload: unknown) {
  for (const listener of [...state.listeners]) listener(payload);
}

beforeEach(() => {
  state.native = { isAvailable: vi.fn(() => true), addListener: vi.fn(), removeListener: vi.fn() };
});

afterEach(() => {
  state.listeners.clear();
  state.event = null;
});

describe('MicButton', () => {
  it('is available where the host has the native module', async () => {
    expect((await loadMicButton()).isAvailable).toBe(true);
  });

  it('is unavailable and does nothing without the native module', async () => {
    state.native = null;
    const MicButton = await loadMicButton();
    const listener = vi.fn();

    const subscription = MicButton.addListener('pressIn', listener);
    send({ type: 'pressIn' });
    subscription.remove();

    expect(MicButton.isAvailable).toBe(false);
    expect(listener).not.toHaveBeenCalled();
  });

  it('holds the button for each listener and hands it back as each is removed', async () => {
    const MicButton = await loadMicButton();

    const pressIn = MicButton.addListener('pressIn', () => undefined);
    const pressOut = MicButton.addListener('pressOut', () => undefined);
    pressIn.remove();
    pressIn.remove();
    pressOut.remove();

    expect(state.native?.addListener).toHaveBeenNthCalledWith(1, 'pressIn');
    expect(state.native?.addListener).toHaveBeenNthCalledWith(2, 'pressOut');
    expect(state.native?.removeListener).toHaveBeenCalledTimes(2);
    expect(state.listeners.size).toBe(0);
  });

  it('calls each listener for its own press only', async () => {
    const MicButton = await loadMicButton();
    const pressIn = vi.fn();
    const pressOut = vi.fn();
    MicButton.addListener('pressIn', pressIn);
    MicButton.addListener('pressOut', pressOut);

    send({ type: 'pressIn' });
    send({ type: 'pressOut' });
    send({ type: 'pressOut' });

    expect(state.event).toBe('rnTizenMicButton');
    expect(pressIn).toHaveBeenCalledTimes(1);
    expect(pressOut).toHaveBeenCalledTimes(2);
  });

  it('ignores a malformed event', async () => {
    const MicButton = await loadMicButton();
    const listener = vi.fn();
    MicButton.addListener('pressIn', listener);

    send({ type: 'hold' });
    send(null);

    expect(listener).not.toHaveBeenCalled();
  });
});
