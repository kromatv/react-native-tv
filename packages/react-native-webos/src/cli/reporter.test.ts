import { afterEach, describe, expect, it, vi } from 'vitest';
import { capturedOutput } from '../fakes.fixtures.ts';
import { Reporter } from './reporter.ts';

const PLAIN = { NO_COLOR: '1' };

afterEach(() => {
  vi.useRealTimers();
});

describe('Reporter', () => {
  it('prints a finished step with its summary', async () => {
    const out = capturedOutput();
    const reporter = new Reporter({ verbose: false, out, env: PLAIN });

    const result = await reporter.step(
      'Bundle',
      async () => 42,
      (value) => `${value} files`,
    );

    expect(result).toBe(42);
    expect(out.text()).toMatch(/^✓ Bundle {17}42 files {2}\d+\.\ds\n$/);
  });

  it('marks a failed step and rethrows what failed', async () => {
    const out = capturedOutput();
    const reporter = new Reporter({ verbose: false, out, env: PLAIN });

    const failing = reporter.step('Sign', async () => {
      throw new Error('no profile');
    });

    await expect(failing).rejects.toThrow('no profile');
    expect(out.text()).toMatch(/^✗ Sign /);
  });

  it('announces a step before it runs under --verbose', async () => {
    const out = capturedOutput();
    const reporter = new Reporter({ verbose: true, out, env: PLAIN });

    await reporter.step('Build', async () => undefined);

    expect(out.text().split('\n')[0]).toBe('› Build');
  });

  it('animates a step in place on a terminal and shows its progress', async () => {
    vi.useFakeTimers();
    const out = capturedOutput(true);
    const reporter = new Reporter({ verbose: false, out, env: PLAIN });

    await reporter.step('Metro', async (task) => {
      task.progress('bundling');
      await vi.advanceTimersByTimeAsync(200);
    });

    expect(out.text()).toContain('bundling');
    expect(out.text()).toContain('\r\u001b[2K✓ Metro');
  });

  it('prints a line above a running spinner', async () => {
    vi.useFakeTimers();
    const out = capturedOutput(true);
    const reporter = new Reporter({ verbose: false, out, env: PLAIN });

    await reporter.step('Wait', async () => {
      reporter.note('another build is running');
    });

    expect(out.text()).toContain('\r\u001b[2K  another build is running\n');
  });

  it('does not animate in CI', async () => {
    const out = capturedOutput(true);
    const reporter = new Reporter({ verbose: false, out, env: { ...PLAIN, CI: 'true' } });

    await reporter.step('Test', async () => undefined);

    expect(out.text()).not.toContain('\r');
  });

  it('prints a field as an aligned label and value', () => {
    const out = capturedOutput();
    const reporter = new Reporter({ verbose: false, out, env: PLAIN });

    reporter.field('Target', 'armv7l');

    expect(out.text()).toBe(`  ${'Target'.padEnd(22)} armv7l\n`);
  });

  it('runs a task quietly unless --verbose', async () => {
    const out = capturedOutput();
    const reporter = new Reporter({ verbose: false, out, env: PLAIN });

    const output = await reporter.task().run([process.execPath, '-e', "console.log('hidden')"]);

    expect(output).toBe('hidden\n');
    expect(out.text()).toBe('');
  });
});
