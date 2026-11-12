import { tmpdir } from 'node:os';
import { describe, expect, it } from 'vitest';
import { runManager } from './run-manager.ts';

describe('runManager', () => {
  it('gives what the package manager printed', () => {
    expect(runManager('bun', ['--version'], tmpdir())).toMatch(/^\d+\.\d+\.\d+/);
  });

  it('throws the command and what it printed when it fails', () => {
    expect(() => runManager('bun', ['run', 'a-script-nobody-wrote'], tmpdir())).toThrow(
      /^bun run a-script-nobody-wrote failed:\n.*a-script-nobody-wrote/s,
    );
  });

  it('throws the reason when the command cannot start', () => {
    expect(() => runManager('bun', ['--version'], '/no/such/directory')).toThrow(
      /^bun --version failed:\n.*ENOENT/s,
    );
  });
});
