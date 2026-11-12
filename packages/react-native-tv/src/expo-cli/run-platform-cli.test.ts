import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { describe, expect, it } from 'vitest';
import { runPlatformCli, type ScriptResult } from './run-platform-cli.ts';

function ending(result: ScriptResult) {
  const calls: (readonly [string, readonly string[]])[] = [];
  return {
    calls,
    run: (script: string, args: readonly string[]) => {
      calls.push([script, args]);
      return result;
    },
  };
}

describe('runPlatformCli', () => {
  it("runs the command's run with the arguments, and gives its exit code", () => {
    const script = ending({ status: 3, signal: null });

    expect(runPlatformCli('/app/cli.js', ['--device', 'tv'], script.run)).toBe(3);
    expect(script.calls).toEqual([['/app/cli.js', ['run', '--device', 'tv']]]);
  });

  it('gives 128 plus the signal for a command a signal ended', () => {
    expect(runPlatformCli('/app/cli.js', [], ending({ status: null, signal: 'SIGINT' }).run)).toBe(
      130,
    );
    expect(runPlatformCli('/app/cli.js', [], ending({ status: null, signal: null }).run)).toBe(128);
  });

  it('throws when the command could not start', () => {
    const failed = ending({ status: null, signal: null, error: new Error('spawn ENOENT') });

    expect(() => runPlatformCli('/app/cli.js', [], failed.run)).toThrow('spawn ENOENT');
  });

  it('runs the script with this Node', () => {
    const dir = mkdtempSync(join(tmpdir(), 'rn-tv-script-'));
    const script = join(dir, 'cli.js');
    writeFileSync(script, 'process.exit(process.argv[2] === "run" ? 7 : 1);');

    expect(runPlatformCli(script, [])).toBe(7);
    rmSync(dir, { recursive: true, force: true });
  });
});
