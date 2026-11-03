import { describe, expect, it } from 'vitest';
import { chooseInstance, simulatorCommandLine, simulatorInstance } from './instance.ts';

describe('simulatorInstance', () => {
  it('gives the default instance the first display and ports', () => {
    expect(simulatorInstance('default')).toEqual({
      name: 'default',
      display: 100,
      viewerPort: 40000,
      vncPort: 42000,
      logPort: 41000,
    });
  });

  it('derives the same numbers from the same name every time', () => {
    expect(simulatorInstance('agent-a')).toEqual(simulatorInstance('agent-a'));
  });

  it('keeps every derived number in its own range', () => {
    const names = ['agent-a', 'agent-b', 'ui', 'x', 'a-very-long-instance-name-of-32c'];

    for (const instance of names.map(simulatorInstance)) {
      expect(instance.display).toBeGreaterThan(100);
      expect(instance.display).toBeLessThan(1100);
      expect(instance.viewerPort - 40000).toBe(instance.display - 100);
      expect(instance.logPort - 41000).toBe(instance.display - 100);
      expect(instance.vncPort - 42000).toBe(instance.display - 100);
    }
  });

  it('gives different names different displays', () => {
    expect(simulatorInstance('agent-a').display).not.toBe(simulatorInstance('agent-b').display);
  });
});

describe('chooseInstance', () => {
  it('prefers the flag over the environment', () => {
    const env = { RN_TIZEN_SIMULATOR_INSTANCE: 'from-env' };

    expect(chooseInstance('from-flag', env).name).toBe('from-flag');
  });

  it('reads the environment when there is no flag', () => {
    expect(chooseInstance(undefined, { RN_TIZEN_SIMULATOR_INSTANCE: 'agent-a' }).name).toBe(
      'agent-a',
    );
  });

  it('falls back to the default instance', () => {
    expect(chooseInstance(undefined, { RN_TIZEN_SIMULATOR_INSTANCE: '' }).name).toBe('default');
  });

  it('rejects a name that cannot be a directory', () => {
    expect(() => chooseInstance('../app', {})).toThrow(/--instance must be/);
  });

  it('names the environment variable when its value is wrong', () => {
    expect(() => chooseInstance(undefined, { RN_TIZEN_SIMULATOR_INSTANCE: 'Agent A' })).toThrow(
      /RN_TIZEN_SIMULATOR_INSTANCE must be/,
    );
  });
});

describe('simulatorCommandLine', () => {
  it('leaves the default instance out', () => {
    expect(simulatorCommandLine('crash', 'default')).toBe('rn-tizen simulator crash');
  });

  it('names any other instance', () => {
    expect(simulatorCommandLine('crash', 'agent-a')).toBe(
      'rn-tizen simulator crash --instance agent-a',
    );
  });
});
