import { z } from 'zod';
import { LunaError, type LunaReply } from './luna.ts';

const statusSchema = z.object({
  details: z.object({
    state: z.string().max(256),
    reason: z.string().max(1024).optional(),
    packageId: z.string().max(256).optional(),
  }),
});

/** What one status line of the app installer says. */
export interface InstallerStatus {
  readonly state: string;
  readonly reason?: string;
  /** The id of the package being installed, once the installer has read it. */
  readonly packageId?: string;
}

/** The status a reply of a subscribed install or remove carries, or null for a reply without one. */
export function installerStatus(reply: LunaReply): InstallerStatus | null {
  const status = statusSchema.safeParse(reply);
  return status.success ? status.data.details : null;
}

/**
 * Reads one status line as LG's CLI does: true once the state matches `done`,
 * undefined to keep waiting, and the TV's reason thrown when the state says it failed.
 */
export function installerSettled(uri: string, reply: LunaReply, done: RegExp): true | undefined {
  const status = installerStatus(reply);
  if (status === null) return undefined;
  if (/FAILED/i.test(status.state)) throw new LunaError(uri, status.reason ?? status.state);
  return done.test(status.state) ? true : undefined;
}
