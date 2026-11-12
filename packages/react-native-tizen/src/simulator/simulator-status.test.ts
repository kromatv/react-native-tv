import { describe, expect, it } from 'vitest';
import { describeApp, describeRuntime, parseSimulatorStatus } from './simulator-status.ts';

const running =
  '{"name":"default","staged":true,"app":4301,"lastApp":4301,"crashed":false,"screen":4290,"viewer":null,"runtime":"0123456789ab","runtimeBuilt":true}';
const crashed =
  '{"name":"agent-a","staged":true,"app":null,"lastApp":5120,"crashed":true,"screen":5101,"viewer":null,"runtime":"","runtimeBuilt":false}';

describe('parseSimulatorStatus', () => {
  it('reads every instance from the last line the guest script printed', () => {
    const status = parseSimulatorStatus(
      `limactl noise\n{"runtimes":[{"key":"0123456789ab","built":true}],"instances":[${running},${crashed}]}\n`,
    );

    expect(status.instances.map((instance) => instance.name)).toEqual(['default', 'agent-a']);
    expect(status.instances[0]?.app).toBe(4301);
  });

  it('reads the runtime build each instance runs', () => {
    const status = parseSimulatorStatus(
      `{"runtimes":[{"key":"0123456789ab","built":true}],"instances":[${running},${crashed}]}`,
    );

    expect(status.instances.map((instance) => instance.runtime)).toEqual(['0123456789ab', '']);
  });

  it('rejects a runtime key that is not one', () => {
    const status = `{"runtimes":[{"key":"../build","built":true}],"instances":[]}`;

    expect(() => parseSimulatorStatus(status)).toThrow();
  });

  it('reads a VM that holds no instance yet', () => {
    expect(parseSimulatorStatus('{"runtimes":[],"instances":[]}').instances).toEqual([]);
  });

  it('rejects an instance name that is not one', () => {
    const status = `{"runtimes":[{"key":"0123456789ab","built":true}],"instances":[${running.replace('"default"', '"../x"')}]}`;

    expect(() => parseSimulatorStatus(status)).toThrow();
  });
});

describe('describeApp', () => {
  const [defaultInstance, agent] = parseSimulatorStatus(
    `{"runtimes":[{"key":"0123456789ab","built":true}],"instances":[${running},${crashed}]}`,
  ).instances;

  it('points a crashed app at the crash report of its instance', () => {
    expect(describeApp('agent-a', agent)).toBe(
      'crashed (pid 5120): rn-tizen simulator crash --instance agent-a',
    );
  });

  it('offers a restart for an app that exited', () => {
    const stopped = defaultInstance === undefined ? undefined : { ...defaultInstance, app: null };

    expect(describeApp('default', stopped)).toBe('stopped: rn-tizen simulator restart');
  });

  it('offers a run for an instance the VM does not hold', () => {
    expect(describeApp('agent-b', undefined)).toBe(
      'not started: rn-tizen simulator run --instance agent-b',
    );
  });
});

describe('describeRuntime', () => {
  it('names a build by its key', () => {
    expect(describeRuntime('0123456789ab', true)).toBe('0123456789ab');
  });

  it('says when a build is not built', () => {
    expect(describeRuntime('0123456789ab', false)).toBe('0123456789ab, not built');
  });

  it('calls the build from before each app had its own the shared one', () => {
    expect(describeRuntime('', true)).toBe('shared');
  });
});
