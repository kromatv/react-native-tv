import { afterEach, describe, expect, it, vi } from 'vitest';
import { CommandError, lastLines, MissingProgramError, run, runSync } from './exec.ts';

const NODE = process.execPath;

afterEach(() => {
  vi.restoreAllMocks();
});

describe('run', () => {
  it('returns what a quiet command printed on both streams', async () => {
    const output = await run(
      [NODE, '-e', "process.stdout.write('out\\n'); process.stderr.write('err\\n')"],
      { quiet: true },
    );

    expect(output).toContain('out\n');
    expect(output).toContain('err\n');
  });

  it('hands every complete line to onLine', async () => {
    const lines: string[] = [];

    await run([NODE, '-e', "console.log('one'); console.log('two')"], {
      quiet: true,
      onLine: (line) => lines.push(line),
    });

    expect(lines).toEqual(['one', 'two']);
  });

  it('streams to the terminal unless quiet', async () => {
    const write = vi.spyOn(process.stdout, 'write').mockImplementation(() => true);

    await run([NODE, '-e', "console.log('shown')"]);

    expect(write).toHaveBeenCalledWith(expect.anything());
  });

  it('runs in cwd with env', async () => {
    const output = await run([NODE, '-e', 'console.log(process.cwd(), process.env.RN_TIZEN_X)'], {
      quiet: true,
      cwd: '/',
      env: { ...process.env, RN_TIZEN_X: 'set' },
    });

    expect(output.trim()).toBe('/ set');
  });

  it('fails with the exit code and the output of a command that exits non-zero', async () => {
    const failing = run([NODE, '-e', "console.log('why'); process.exit(3)"], { quiet: true });

    await expect(failing).rejects.toBeInstanceOf(CommandError);
    await expect(failing).rejects.toMatchObject({ exitCode: 3, output: 'why\n' });
  });

  it('names the program that is not installed', async () => {
    await expect(run(['rn-tizen-no-such-program'], { quiet: true })).rejects.toEqual(
      new MissingProgramError('rn-tizen-no-such-program'),
    );
  });

  it('refuses a command without a program', async () => {
    await expect(run([])).rejects.toThrow('A command needs a program to run');
  });
});

describe('runSync', () => {
  it('returns the standard output', () => {
    expect(runSync([NODE, '-e', "console.log('sync')"])).toBe('sync\n');
  });

  it('fails with both streams of a command that exits non-zero', () => {
    const failing = () =>
      runSync([NODE, '-e', "console.log('a'); console.error('b'); process.exit(2)"]);

    expect(failing).toThrow(CommandError);
    expect(failing).toThrow(expect.objectContaining({ exitCode: 2, output: 'a\nb\n' }));
  });

  it('names the program that is not installed', () => {
    expect(() => runSync(['rn-tizen-no-such-program'])).toThrow(MissingProgramError);
  });

  it('fails when the command outlives its timeout', () => {
    expect(() => runSync([NODE, '-e', 'setTimeout(() => {}, 5000)'], { timeoutMs: 100 })).toThrow();
  });
});

describe('lastLines', () => {
  it('keeps the last non-blank lines', () => {
    expect(lastLines('a\n\nb\r\nc\n  \n', 2)).toEqual(['b', 'c']);
  });
});
