import { z } from 'zod';

// LG reserves these for its own apps, and the TV's app manager refuses them from
// the developer path (docs/reference/webos-tv-platform.md, sections 1.3 and 2).
const RESERVED_PREFIXES = ['com.palm', 'com.webos', 'com.lge', 'com.palmdts'] as const;

/** A webOS app id, by LG's rules for `id` in `appinfo.json`. */
export const appIdSchema = z
  .string()
  .regex(
    /^[a-z0-9][a-z0-9.-]+$/,
    'expected an id such as com.example.myapp: lowercase letters, digits, periods and minus signs, starting with a letter or a digit',
  )
  .refine((id) => !RESERVED_PREFIXES.some((prefix) => id.startsWith(prefix)), {
    message: `must not start with ${RESERVED_PREFIXES.join(', ')}: LG keeps those for its own apps`,
  });

/** A webOS app version: three numbers of up to nine digits, none with a leading zero. */
export const appVersionSchema = z
  .string()
  .regex(
    /^([1-9]\d{0,8}|\d)\.([1-9]\d{0,8}|\d)\.([1-9]\d{0,8}|\d)$/,
    'expected a version such as 1.0.0: three numbers, none with a leading zero',
  );
