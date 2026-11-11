import { z } from 'zod';
import { UsageError } from '../cli/command.ts';

/**
 * The graphics planes a webOS TV gives an app: 1920x1080 on UHD models,
 * 1280x720 on Full HD ones (docs/reference/webos-tv-platform.md, section 6).
 */
export const SCREEN_SIZES = ['1920x1080', '1280x720'] as const;
export type ScreenSize = (typeof SCREEN_SIZES)[number];

const screenSizeSchema = z.enum(SCREEN_SIZES);

/** `--screen` as a size the simulator can show, or undefined to keep the instance's. */
export function screenSize(flag: string | undefined): ScreenSize | undefined {
  if (flag === undefined) return undefined;
  const parsed = screenSizeSchema.safeParse(flag);
  if (parsed.success) return parsed.data;
  throw new UsageError(`--screen takes ${SCREEN_SIZES.join(' or ')}, got ${flag}`, 'simulator');
}
