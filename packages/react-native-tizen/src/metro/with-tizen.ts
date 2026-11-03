import { existsSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, extname, join, relative, sep } from 'node:path';
import { fileURLToPath } from 'node:url';
import { UPSTREAM_TRANSFORMER_ENV } from './tizen-babel-transformer-env.ts';

/** The platform name a Tizen bundle is requested with. */
export const TIZEN_PLATFORM = 'tizen';

/** The platform whose sources a Tizen bundle falls back to. */
export const FALLBACK_PLATFORM = 'android';

const OVERRIDES_DIR = fileURLToPath(new URL('../../react-native-overrides/', import.meta.url));
const TIZEN_BABEL_TRANSFORMER = fileURLToPath(
  new URL(`./tizen-babel-transformer${extname(import.meta.url)}`, import.meta.url),
);
const OVERRIDE_EXTENSIONS = ['.ts', '.js'] as const;

type Resolution =
  | { readonly type: 'sourceFile'; readonly filePath: string }
  | { readonly type: 'assetFiles'; readonly filePaths: readonly string[] }
  | { readonly type: 'empty' };

interface ResolutionContext {
  readonly resolveRequest: ResolveRequest;
  readonly originModulePath?: string;
}

type ResolveRequest = (
  context: ResolutionContext,
  moduleName: string,
  platform: string | null,
) => Resolution;

/** The part of a Metro config {@link withTizen} reads and extends. */
export interface MetroConfigLike {
  readonly projectRoot?: string;
  readonly resolver?: {
    readonly platforms?: readonly string[];
    readonly resolveRequest?: ResolveRequest | null;
    readonly [key: string]: unknown;
  };
  readonly transformer?: {
    readonly babelTransformerPath?: string;
    readonly [key: string]: unknown;
  };
  readonly [key: string]: unknown;
}

const SOURCE_FILE = /^(.*?)(?:\.(?:android|native))?(\.[cm]?[jt]sx?)$/;
const TWIN_EXTENSIONS = ['.ts', '.tsx', '.js', '.jsx', '.mjs', '.cjs'] as const;

function tizenTwin(filePath: string): string | null {
  const match = SOURCE_FILE.exec(filePath);
  if (match === null) return null;
  const stem = match[1] ?? '';
  if (stem.endsWith(`.${TIZEN_PLATFORM}`)) return null;
  for (const extension of TWIN_EXTENSIONS) {
    const candidate = `${stem}.${TIZEN_PLATFORM}${extension}`;
    if (existsSync(candidate)) return candidate;
  }
  return null;
}

function overrideFor(reactNativeDir: string, filePath: string): string | null {
  const fromReactNative = relative(reactNativeDir, filePath);
  if (fromReactNative.startsWith('..') || fromReactNative.includes(`node_modules${sep}`))
    return null;
  const stem = fromReactNative
    .replace(/\.android\.[cm]?[jt]sx?$/, '')
    .replace(/\.[cm]?[jt]sx?$/, '');
  for (const extension of OVERRIDE_EXTENSIONS) {
    const candidate = join(OVERRIDES_DIR, `${stem}.${TIZEN_PLATFORM}${extension}`);
    if (existsSync(candidate)) return candidate;
  }
  return null;
}

function wrapBabelTransformer(upstream: string | undefined): string | undefined {
  if (upstream === undefined || upstream === TIZEN_BABEL_TRANSFORMER) return upstream;
  process.env[UPSTREAM_TRANSFORMER_ENV] = upstream;
  return TIZEN_BABEL_TRANSFORMER;
}

function resolveReactNativeDir(projectRoot: string): string {
  return dirname(
    createRequire(join(projectRoot, 'package.json')).resolve('react-native/package.json'),
  );
}

/**
 * Teaches a Metro config (Expo's `getDefaultConfig`, or React Native's) the
 * `tizen` platform. A module resolves as it would for Android, which is what
 * React Native's own sources are closest to on a TV; then a `.tizen.*` file
 * beside the file found wins, and the few React Native files Tizen replaces
 * are swapped for this package's versions, except in the twin or replacement
 * itself, which may import the file it wraps. The platform is never handed to the
 * resolvers underneath, because Expo's own throws for a platform it does not
 * know. Its Babel transformer is wrapped so that a `Platform.select`
 * Expo inlines picks `android` for Tizen, as it does at run time. Other
 * platforms resolve and transform exactly as before.
 */
export function withTizen<Config extends MetroConfigLike>(config: Config): Config {
  const projectRoot = config.projectRoot ?? process.cwd();
  const reactNativeDir = resolveReactNativeDir(projectRoot);
  const upstream = config.resolver?.resolveRequest ?? null;
  const platforms = config.resolver?.platforms ?? ['ios', 'android'];

  const resolveRequest: ResolveRequest = (context, moduleName, platform) => {
    const next = upstream ?? context.resolveRequest;
    if (platform !== TIZEN_PLATFORM) return next(context, moduleName, platform);

    const fallback = next(context, moduleName, FALLBACK_PLATFORM);
    if (fallback.type !== 'sourceFile') return fallback;
    const twin = tizenTwin(fallback.filePath);
    if (twin !== null && twin !== context.originModulePath)
      return { type: 'sourceFile', filePath: twin };
    const override = overrideFor(reactNativeDir, fallback.filePath);
    if (override === null || override === context.originModulePath) return fallback;
    return { type: 'sourceFile', filePath: override };
  };

  const babelTransformerPath = wrapBabelTransformer(config.transformer?.babelTransformerPath);
  return {
    ...config,
    ...(babelTransformerPath === undefined
      ? {}
      : { transformer: { ...config.transformer, babelTransformerPath } }),
    resolver: {
      ...config.resolver,
      platforms: platforms.includes(TIZEN_PLATFORM) ? platforms : [...platforms, TIZEN_PLATFORM],
      resolveRequest,
    },
  };
}
