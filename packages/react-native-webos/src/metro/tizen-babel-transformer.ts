import { createRequire } from 'node:module';
import { sep } from 'node:path';
import { z } from 'zod';
import {
  INTL_LOCALE_DATA_MODULE,
  intlLocaleDataSource,
  parseIntlLocales,
} from './intl-locale-data.ts';
import { INTL_LOCALES_ENV, UPSTREAM_TRANSFORMER_ENV } from './tizen-babel-transformer-env.ts';
import { tizenAndroidBranchPlugin } from './tizen-android-branch.ts';
import { tizenListFocusGuidePlugin } from './tizen-list-focus-guide.ts';
import { tizenPlatformSelectPlugin } from './tizen-platform-select.ts';

const CACHE_VERSION = 'rn-tizen-transform-3';

interface TransformArgs {
  readonly filename?: string;
  readonly src?: string;
  readonly options: { readonly platform?: string | null };
  readonly plugins?: readonly unknown[];
  readonly [key: string]: unknown;
}

const callable = <Signature>() => z.custom<Signature>((value) => typeof value === 'function');

const upstreamSchema = z.object({
  transform: callable<(args: TransformArgs) => unknown>(),
  getCacheKey: callable<(...args: unknown[]) => string>().optional(),
});

type Upstream = z.infer<typeof upstreamSchema>;

function loadUpstream(): Upstream {
  const path = process.env[UPSTREAM_TRANSFORMER_ENV];
  if (path === undefined || path === '') {
    throw new Error(
      `${UPSTREAM_TRANSFORMER_ENV} is not set: load this transformer through withTizen()`,
    );
  }
  return upstreamSchema.parse(createRequire(import.meta.url)(path));
}

const upstream = loadUpstream();

function intlLocales(): string[] {
  return parseIntlLocales(process.env[INTL_LOCALES_ENV]);
}

function tizenSource(args: TransformArgs): string | undefined {
  const filename = args.filename?.split(sep).join('/');
  return filename?.endsWith(INTL_LOCALE_DATA_MODULE)
    ? intlLocaleDataSource(intlLocales())
    : args.src;
}

/**
 * The app's Babel transformer, with {@link tizenPlatformSelectPlugin},
 * {@link tizenAndroidBranchPlugin} and {@link tizenListFocusGuidePlugin} added
 * for a Tizen bundle, and the Intl
 * locale data module written for the locales `RN_TIZEN_INTL_LOCALES` names.
 * Every other platform goes through it unchanged.
 */
export function transform(args: TransformArgs): unknown {
  if (args.options.platform !== 'tizen') return upstream.transform(args);
  return upstream.transform({
    ...args,
    src: tizenSource(args),
    plugins: [
      ...(args.plugins ?? []),
      tizenPlatformSelectPlugin,
      tizenAndroidBranchPlugin,
      tizenListFocusGuidePlugin,
    ],
  });
}

/** The app's transformer's cache key, changed whenever this wrapper's output does. */
export function getCacheKey(...args: unknown[]): string {
  return `${upstream.getCacheKey?.(...args) ?? ''}$${CACHE_VERSION}$${intlLocales().join(',')}`;
}
