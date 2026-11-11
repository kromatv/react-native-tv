import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { UPSTREAM_TRANSFORMER_ENV } from './webos-babel-transformer-env.ts';
import { withWebOS, type MetroConfigLike } from './with-webos.ts';

const projectRoot = fileURLToPath(new URL('../../example/', import.meta.url));
const wrapper = fileURLToPath(new URL('./webos-babel-transformer.ts', import.meta.url));

describe('withWebOS', () => {
  it('wraps the app Babel transformer and remembers it for the workers', () => {
    const config = withWebOS({
      projectRoot,
      transformer: { babelTransformerPath: '/app/transformer.js' },
    });

    expect(config.transformer?.babelTransformerPath).toBe(wrapper);
    expect(process.env[UPSTREAM_TRANSFORMER_ENV]).toBe('/app/transformer.js');
  });

  it('wraps it once when applied twice', () => {
    const config = withWebOS(
      withWebOS({ projectRoot, transformer: { babelTransformerPath: '/app/transformer.js' } }),
    );

    expect(config.transformer?.babelTransformerPath).toBe(wrapper);
    expect(process.env[UPSTREAM_TRANSFORMER_ENV]).toBe('/app/transformer.js');
  });

  it('adds webos to the platforms Metro knows', () => {
    const config: MetroConfigLike = { projectRoot };

    expect(withWebOS(config).resolver?.platforms).toEqual(['ios', 'android', 'webos']);
  });
});

describe('withWebOS, resolving React Native for webos', () => {
  const reactNativeDir = dirname(
    createRequire(join(projectRoot, 'package.json')).resolve('react-native/package.json'),
  );
  const original = join(reactNativeDir, 'Libraries', 'Alert', 'Alert.js');
  const override = fileURLToPath(
    new URL('../../react-native-overrides/Libraries/Alert/Alert.webos.ts', import.meta.url),
  );
  const resolveRequest = (_context: unknown, _name: string, _platform: string | null) =>
    ({ type: 'sourceFile', filePath: original }) as const;
  const resolve = withWebOS({ projectRoot, resolver: { resolveRequest } }).resolver?.resolveRequest;

  it('swaps a file webOS replaces for its override', () => {
    const resolved = resolve?.({ resolveRequest, originModulePath: '/app/index.js' }, 'x', 'webos');

    expect(resolved).toEqual({ type: 'sourceFile', filePath: override });
  });

  it('gives the override itself the file it replaces', () => {
    const resolved = resolve?.({ resolveRequest, originModulePath: override }, 'x', 'webos');

    expect(resolved).toEqual({ type: 'sourceFile', filePath: original });
  });

  it('leaves another platform alone', () => {
    const resolved = resolve?.({ resolveRequest, originModulePath: '/app/index.js' }, 'x', 'ios');

    expect(resolved).toEqual({ type: 'sourceFile', filePath: original });
  });
});

describe('withWebOS, resolving a webOS twin', () => {
  let dir: string;

  beforeEach(() => {
    dir = mkdtempSync(join(tmpdir(), 'rn-webos-twin-'));
  });

  afterEach(() => {
    rmSync(dir, { recursive: true, force: true });
  });

  function resolvedFor(found: string, platform = 'webos') {
    const resolveRequest = (_context: unknown, _name: string, asked: string | null) => {
      if (asked !== 'android' && asked !== 'ios') throw new Error(`asked for ${String(asked)}`);
      return { type: 'sourceFile', filePath: found } as const;
    };
    const resolve = withWebOS({ projectRoot, resolver: { resolveRequest } }).resolver
      ?.resolveRequest;
    return resolve?.({ resolveRequest, originModulePath: '/app/index.js' }, './masked', platform);
  }

  it('prefers a .webos file beside the one Android resolves to', () => {
    writeFileSync(join(dir, 'masked.tsx'), '');
    writeFileSync(join(dir, 'masked.webos.tsx'), '');

    expect(resolvedFor(join(dir, 'masked.tsx'))).toEqual({
      type: 'sourceFile',
      filePath: join(dir, 'masked.webos.tsx'),
    });
  });

  it('prefers it over an .android file too', () => {
    writeFileSync(join(dir, 'masked.android.js'), '');
    writeFileSync(join(dir, 'masked.webos.js'), '');

    expect(resolvedFor(join(dir, 'masked.android.js'))).toEqual({
      type: 'sourceFile',
      filePath: join(dir, 'masked.webos.js'),
    });
  });

  it('never hands the webos platform to the resolvers underneath', () => {
    writeFileSync(join(dir, 'masked.ts'), '');

    expect(resolvedFor(join(dir, 'masked.ts'))).toEqual({
      type: 'sourceFile',
      filePath: join(dir, 'masked.ts'),
    });
  });

  it('gives the twin itself the file it wraps', () => {
    writeFileSync(join(dir, 'masked.tsx'), '');
    writeFileSync(join(dir, 'masked.webos.tsx'), '');
    const resolveRequest = () =>
      ({ type: 'sourceFile', filePath: join(dir, 'masked.tsx') }) as const;
    const resolve = withWebOS({ projectRoot, resolver: { resolveRequest } }).resolver
      ?.resolveRequest as (context: object, name: string, platform: string) => unknown;

    const resolved = resolve(
      { resolveRequest, originModulePath: join(dir, 'masked.webos.tsx') },
      './masked',
      'webos',
    );

    expect(resolved).toEqual({ type: 'sourceFile', filePath: join(dir, 'masked.tsx') });
  });

  it('keeps the file for another platform', () => {
    writeFileSync(join(dir, 'masked.tsx'), '');
    writeFileSync(join(dir, 'masked.webos.tsx'), '');

    expect(resolvedFor(join(dir, 'masked.tsx'), 'ios')).toEqual({
      type: 'sourceFile',
      filePath: join(dir, 'masked.tsx'),
    });
  });
});
