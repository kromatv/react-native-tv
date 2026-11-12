import { existsSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, extname, join, relative, sep } from 'node:path';
import { fileURLToPath } from 'node:url';
import { appIntlLocales } from './app-locales.ts';
import { INTL_LOCALES_ENV, UPSTREAM_TRANSFORMER_ENV } from './webos-babel-transformer-env.ts';
import { LIST_FOCUS_GUIDE_STYLE_MODULE } from './webos-list-focus-guide.ts';

/** The platform name a webOS bundle is requested with. */
export const WEBOS_PLATFORM = 'webos';

/** The platform whose sources a webOS bundle falls back to. */
export const FALLBACK_PLATFORM = 'android';

const OVERRIDES_DIR = fileURLToPath(new URL('../../react-native-overrides/', import.meta.url));
const WEBOS_BABEL_TRANSFORMER = fileURLToPath(
  new URL(`./webos-babel-transformer${extname(import.meta.url)}`, import.meta.url),
);
const OVERRIDE_EXTENSIONS = ['.ts', '.js'] as const;
const LIST_FOCUS_GUIDE_STYLE_FILE = join(
  OVERRIDES_DIR,
  'Libraries',
  'Lists',
  'list-focus-guide-style.ts',
);

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

/** The part of a Metro config {@link withWebOS} reads and extends. */
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

function webosTwin(filePath: string): string | null {
  const match = SOURCE_FILE.exec(filePath);
  if (match === null) return null;
  const stem = match[1] ?? '';
  if (stem.endsWith(`.${WEBOS_PLATFORM}`)) return null;
  for (const extension of TWIN_EXTENSIONS) {
    const candidate = `${stem}.${WEBOS_PLATFORM}${extension}`;
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
    const candidate = join(OVERRIDES_DIR, `${stem}.${WEBOS_PLATFORM}${extension}`);
    if (existsSync(candidate)) return candidate;
  }
  return null;
}

function wrapBabelTransformer(upstream: string | undefined): string | undefined {
  if (upstream === undefined || upstream === WEBOS_BABEL_TRANSFORMER) return upstream;
  process.env[UPSTREAM_TRANSFORMER_ENV] = upstream;
  return WEBOS_BABEL_TRANSFORMER;
}

function resolveReactNativeDir(projectRoot: string): string {
  return dirname(
    createRequire(join(projectRoot, 'package.json')).resolve('react-native/package.json'),
  );
}

/**
 * Teaches a Metro config (Expo's `getDefaultConfig`, or React Native's) the
 * `webos` platform. A module resolves as it would for Android, which is what
 * React Native's own sources are closest to on a TV; then a `.webos.*` file
 * beside the file found wins, and the few React Native files webOS replaces
 * are swapped for this package's versions, except in the twin or replacement
 * itself, which may import the file it wraps. The platform is never handed to the
 * resolvers underneath, because Expo's own throws for a platform it does not
 * know. Its Babel transformer is wrapped so that a `Platform.select`
 * Expo inlines picks `android` for webOS, as it does at run time. Other
 * platforms resolve and transform exactly as before.
 */
export function withWebOS<Config extends MetroConfigLike>(config: Config): Config {
  const projectRoot = config.projectRoot ?? process.cwd();
  const reactNativeDir = resolveReactNativeDir(projectRoot);
  if (process.env[INTL_LOCALES_ENV] === undefined) {
    const locales = appIntlLocales(projectRoot);
    if (locales !== null) process.env[INTL_LOCALES_ENV] = locales.join(',');
  }
  const upstream = config.resolver?.resolveRequest ?? null;
  const platforms = config.resolver?.platforms ?? ['ios', 'android'];

  const resolveRequest: ResolveRequest = (context, moduleName, platform) => {
    const next = upstream ?? context.resolveRequest;
    if (platform !== WEBOS_PLATFORM) return next(context, moduleName, platform);
    if (moduleName === LIST_FOCUS_GUIDE_STYLE_MODULE)
      return { type: 'sourceFile', filePath: LIST_FOCUS_GUIDE_STYLE_FILE };

    const fallback = next(context, moduleName, FALLBACK_PLATFORM);
    if (fallback.type !== 'sourceFile') return fallback;
    const twin = webosTwin(fallback.filePath);
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
      platforms: platforms.includes(WEBOS_PLATFORM) ? platforms : [...platforms, WEBOS_PLATFORM],
      resolveRequest,
    },
  };
}
