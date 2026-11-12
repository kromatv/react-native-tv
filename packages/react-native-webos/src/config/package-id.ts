import { z } from 'zod';
import { appIdSchema } from './app-id.ts';

const FALLBACK_PACKAGE_ID = 'com.anonymous.app';

const expoIdsSchema = z.looseObject({
  slug: z.string().optional(),
  android: z.looseObject({ package: z.string().optional() }).optional(),
});

function sanitized(value: string): string {
  return value
    .toLowerCase()
    .replaceAll(/[^a-z0-9.-]/g, '')
    .replaceAll(/\.+/g, '.')
    .replaceAll(/^\.|\.$/g, '');
}

/** Why `id` cannot be a webOS app id, or null when it can. */
export function packageIdIssue(id: string): string | null {
  const parsed = appIdSchema.safeParse(id);
  return parsed.success ? null : (parsed.error.issues[0]?.message ?? 'not an app id');
}

/**
 * The app id `rn-webos init` offers for an Expo config: its Android package
 * when webOS takes it, else `com.anonymous.<slug>`, as `expo prebuild`
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
