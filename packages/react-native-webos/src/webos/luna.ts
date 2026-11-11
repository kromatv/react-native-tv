import { z, type ZodType } from 'zod';
import type { ExecResult, SshSession } from './ssh-session.ts';

const LUNA_SEND = '/usr/bin/luna-send-pub';
const CALL_TIMEOUT_MS = 30_000;
const MAX_REPLY_CHARS = 512 * 1024;

const replySchema = z.record(z.string(), z.unknown());
const failureSchema = z.object({
  errorText: z.string().max(4096).optional(),
  errorMessage: z.string().max(4096).optional(),
});

/** One reply of a luna service, as `luna-send-pub` printed it. */
export type LunaReply = z.infer<typeof replySchema>;

/** A luna service that answered `returnValue: false`, with its own words. */
export class LunaError extends Error {
  readonly reason: string;

  constructor(uri: string, reason: string) {
    super(`${uri} answered: ${reason}`);
    this.name = 'LunaError';
    this.reason = reason;
  }
}

/** A command on the TV that failed, or printed nothing `rn-webos` can read. */
export class TvCommandError extends Error {
  readonly output: string;

  constructor(message: string, result: ExecResult) {
    super(message);
    this.name = 'TvCommandError';
    this.output = `${result.stdout}${result.stderr}`.trim();
  }
}

/** `text` as one word of a POSIX shell command. */
export function shellQuote(text: string): string {
  const escaped = text.replaceAll("'", String.raw`'\''`);
  return `'${escaped}'`;
}

/** The command line of one luna call: `-n 1` for one reply, `-i` to stay subscribed. */
export function lunaCommand(mode: '-n 1' | '-i', uri: string, params: object): string {
  return `${LUNA_SEND} ${mode} ${uri} ${shellQuote(JSON.stringify(params))}`;
}

/** A line `luna-send-pub` printed, as the JSON object it holds; null for anything else. */
export function parseReply(line: string): LunaReply | null {
  if (line.length > MAX_REPLY_CHARS) return null;
  try {
    const parsed = replySchema.safeParse(JSON.parse(line));
    return parsed.success ? parsed.data : null;
  } catch {
    return null;
  }
}

function refusal(uri: string, reply: LunaReply): LunaError | null {
  if (reply.returnValue !== false) return null;
  const parsed = failureSchema.safeParse(reply);
  const reason = parsed.success ? (parsed.data.errorText ?? parsed.data.errorMessage) : undefined;
  return new LunaError(uri, reason ?? 'no reason given');
}

function silent(uri: string, result: ExecResult): TvCommandError {
  const said = `${result.stdout}${result.stderr}`.trim();
  const detail = said === '' ? '' : `: ${said}`;
  return new TvCommandError(`${uri} did not answer${detail}`, result);
}

/** Calls a luna service for one reply and holds it to `schema`. */
export async function lunaCall<T>(
  session: SshSession,
  uri: string,
  params: object,
  schema: ZodType<T>,
  timeoutMs = CALL_TIMEOUT_MS,
): Promise<T> {
  const result = await session.exec(lunaCommand('-n 1', uri, params), { timeoutMs });
  for (const line of result.stdout.split(/\r?\n/)) {
    const reply = parseReply(line);
    if (reply === null) continue;
    const refused = refusal(uri, reply);
    if (refused !== null) throw refused;
    const parsed = schema.safeParse(reply);
    if (parsed.success) return parsed.data;
    throw new TvCommandError(`${uri} answered something unexpected`, result);
  }
  throw silent(uri, result);
}

/**
 * Subscribes to a luna service and hands each reply to `settle` until it
 * returns a value or throws, then ends the call: `luna-send-pub -i` never
 * ends by itself.
 */
export async function lunaFollow<T>(
  session: SshSession,
  uri: string,
  params: object,
  settle: (reply: LunaReply) => T | undefined,
  timeoutMs: number,
): Promise<T> {
  const state: { outcome: { readonly value: T } | { readonly error: unknown } | null } = {
    outcome: null,
  };
  const onLine = (line: string): boolean => {
    const reply = state.outcome === null ? parseReply(line) : null;
    if (reply === null) return state.outcome !== null;
    try {
      const refused = refusal(uri, reply);
      if (refused !== null) throw refused;
      const value = settle(reply);
      if (value !== undefined) state.outcome = { value };
    } catch (error) {
      state.outcome = { error };
    }
    return state.outcome !== null;
  };
  const result = await session.exec(lunaCommand('-i', uri, params), { timeoutMs, onLine });
  if (state.outcome === null) throw silent(uri, result);
  if ('error' in state.outcome) throw state.outcome.error;
  return state.outcome.value;
}
