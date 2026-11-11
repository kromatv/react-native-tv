import { afterEach, describe, expect, it, vi } from 'vitest';
import { CommandError, run } from '../exec.ts';
import { arpHosts } from './arp.ts';

vi.mock('../exec.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('../exec.ts')>()),
  run: vi.fn(),
}));

afterEach(() => {
  vi.clearAllMocks();
});

describe('arpHosts', () => {
  it('lists the private addresses arp -an knows, quietly', async () => {
    vi.mocked(run).mockResolvedValue(
      '? (192.168.1.107) at a4:30:7a:1:2:3 on en0 ifscope [ethernet]\n',
    );

    expect(await arpHosts()).toEqual(['192.168.1.107']);
    expect(run).toHaveBeenCalledWith(['arp', '-an'], { quiet: true });
  });

  it('is empty when arp fails or is missing', async () => {
    vi.mocked(run).mockRejectedValue(new CommandError(['arp', '-an'], 1, ''));

    expect(await arpHosts()).toEqual([]);
  });
});
