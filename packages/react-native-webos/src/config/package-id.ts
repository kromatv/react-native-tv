import { z } from 'zod';
import { packageIdSchema } from './tizen-config.ts';

const FALLBACK_PACKAGE_ID = 'com.anonymous.app';

const expoIdsSchema = z.looseObject({
  slug: z.string().optional(),
  android: z.looseObject({ package: z.string().optional() }).optional(),
});

/** Expo's sanitizing of a suggested package name: illegal characters and empty segments dropped. */
function sanitized(value: string): string {
  const legal =
    value
      .replaceAll(/[^\w.]/g, '')
      .replaceAll(/\.+/g, '.')
      .replaceAll(/^\.|\.$/g, '') || 'app';
  const segments = legal.split('.');
  const padded = segments.length < 2 ? ['com', ...segments] : segments;
  return padded.map((segment) => (/^[A-Za-z]/.test(segment) ? segment : `x${segment}`)).join('.');
}

/** Why `id` cannot be a Tizen package id, or null when it can. */
export function packageIdIssue(id: string): string | null {
  const parsed = packageIdSchema.safeParse(id);
  return parsed.success ? null : (parsed.error.issues[0]?.message ?? 'not a package id');
}

/**
 * The package id `rn-tizen init` offers for an Expo config: its Android
 * package when Tizen takes it, else `com.anonymous.<slug>`, as `expo prebuild`
 * suggests for an owner it does not know.
 */
export function suggestedPackageId(expo: unknown): string {
  const ids = expoIdsSchema.safeParse(expo);
  const android = ids.success ? ids.data.android?.package : undefined;
  const slug = ids.success ? ids.data.slug : undefined;
  const candidates = [android, slug === undefined ? undefined : sanitized(`com.anonymous.${slug}`)];
  return (
    candidates.find((candidate) => candidate !== undefined && packageIdIssue(candidate) === null) ??
    FALLBACK_PACKAGE_ID
  );
}
