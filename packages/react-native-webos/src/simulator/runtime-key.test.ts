import { describe, expect, it } from 'vitest';
import { RUNTIME_KEY, simulatorRuntimeKey, type RuntimeCode } from './runtime-key.ts';

const reactNative = {
  name: 'react-native',
  version: '0.88.0',
  dir: '/app/node_modules/react-native',
};
const svg = {
  name: 'react-native-svg',
  version: '15.0.0',
  dir: '/app/node_modules/react-native-svg',
  implementation: '/rnt/runtime/host/src/svg/CMakeLists.txt',
};
const discovery = {
  name: 'server-discovery',
  version: '1.0.0',
  dir: '/app/modules/server-discovery',
  implementation: '/app/modules/server-discovery/tizen/CMakeLists.txt',
};

const code = (libraries: RuntimeCode['libraries']): RuntimeCode => ({
  packages: [reactNative],
  libraries,
});

describe('simulatorRuntimeKey', () => {
  it('is twelve hex digits', () => {
    expect(simulatorRuntimeKey(code([svg]))).toMatch(RUNTIME_KEY);
  });

  it('gives two apps with the same native code one build', () => {
    expect(simulatorRuntimeKey(code([svg, discovery]))).toBe(
      simulatorRuntimeKey(code([discovery, svg])),
    );
  });

  it('gives an app with one more native library its own build', () => {
    expect(simulatorRuntimeKey(code([svg, discovery]))).not.toBe(simulatorRuntimeKey(code([svg])));
  });

  it('changes with a library version or the implementation compiled for it', () => {
    const key = simulatorRuntimeKey(code([svg]));

    expect(simulatorRuntimeKey(code([{ ...svg, version: '15.1.0' }]))).not.toBe(key);
    expect(simulatorRuntimeKey(code([{ ...svg, implementation: null }]))).not.toBe(key);
  });

  it('changes with the react-native the runtime is compiled against', () => {
    const other: RuntimeCode = {
      packages: [{ ...reactNative, dir: '/b/react-native' }],
      libraries: [],
    };

    expect(simulatorRuntimeKey(other)).not.toBe(simulatorRuntimeKey(code([])));
  });
});
