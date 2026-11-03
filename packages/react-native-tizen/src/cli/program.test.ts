import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { capturedOutput, type CapturedOutput } from '../fakes.fixtures.ts';
import { runCli } from './program.ts';

let out: CapturedOutput;
let err: CapturedOutput;
let empty: string;

beforeEach(() => {
  out = capturedOutput();
  err = capturedOutput();
  empty = mkdtempSync(join(tmpdir(), 'rn-tizen-cli-'));
});

afterEach(() => {
  rmSync(empty, { recursive: true, force: true });
});

describe('runCli', () => {
  it('lists every command with no arguments', async () => {
    expect(await runCli([], { out, err })).toBe(0);

    expect(out.text()).toContain('Usage: rn-tizen <command> [options]');
    expect(out.text()).toContain('simulator');
  });

  it("prints this package's version", async () => {
    expect(await runCli(['--version'], { out, err })).toBe(0);

    expect(out.text()).toMatch(/^\d+\.\d+\.\d+/);
  });

  it('prints the help of one command, asked either way', async () => {
    await runCli(['help', 'bundle'], { out, err });
    await runCli(['bundle', '--help'], { out, err });

    expect(out.text().match(/--out <dir>/g)).toHaveLength(2);
  });

  it('suggests the command a typo was meant to be, as a usage error', async () => {
    expect(await runCli(['bundel'], { out, err })).toBe(2);

    expect(err.text()).toContain('Unknown command bundel. Did you mean bundle?');
    expect(err.text()).toContain('rn-tizen --help lists what it takes.');
  });

  it('prints what to do about a fixable failure', async () => {
    expect(await runCli(['bundle', '--project', empty], { out, err })).toBe(1);

    expect(err.text()).toContain('is not an app: it has no package.json');
    expect(err.text()).toContain('pass --project <dir>');
  });
});
