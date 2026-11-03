import { beforeEach, describe, expect, it, vi } from 'vitest';

interface FakePreview {
  isAvailable(): boolean;
  launchedFromTile(): boolean;
  publish(json: string): Promise<string>;
  setSource(json: string | null): void;
}

const state = vi.hoisted(() => {
  const shared: { native: FakePreview | null } = { native: null };
  return shared;
});

vi.mock('react-native', () => ({ TurboModuleRegistry: { get: () => state.native } }));

async function loadPreview() {
  vi.resetModules();
  return (await import('./preview.ts')).SmartHubPreview;
}

beforeEach(() => {
  state.native = {
    isAvailable: vi.fn(() => true),
    launchedFromTile: vi.fn(() => true),
    publish: vi.fn(async () => 'ContentDBAPI'),
    setSource: vi.fn(),
  };
});

describe('SmartHubPreview', () => {
  it("publishes Samsung's JSON through the native module", async () => {
    const preview = await loadPreview();

    await expect(preview.clear()).resolves.toEqual({ published: true, channel: 'ContentDBAPI' });
    expect(state.native?.publish).toHaveBeenCalledWith('{"sections":[]}');
    expect(preview.isAvailable).toBe(true);
    expect(preview.launchedFromTile).toBe(true);
  });

  it('resolves with the reason a publish failed', async () => {
    vi.mocked(state.native?.publish ?? vi.fn()).mockRejectedValue(new Error('quota'));
    const preview = await loadPreview();

    await expect(preview.clear()).resolves.toEqual({ published: false, reason: 'quota' });
  });

  it('hands the native module the source, or null to stop fetching', async () => {
    const preview = await loadPreview();

    preview.setSource(null);

    expect(state.native?.setSource).toHaveBeenCalledWith(null);
  });

  it('publishes nothing without a Smart Hub', async () => {
    state.native = null;
    const preview = await loadPreview();

    const result = await preview.clear();

    expect(result.published).toBe(false);
    expect(preview.isAvailable).toBe(false);
    expect(preview.launchedFromTile).toBe(false);
  });
});
