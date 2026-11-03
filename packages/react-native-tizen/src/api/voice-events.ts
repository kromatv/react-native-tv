import { z } from 'zod';

/** What the screen shows, which Samsung's assistant reads before each utterance. */
export type VoiceScreen = 'home' | 'list' | 'details' | 'player' | 'search' | 'settings';

/** A voice search: "search Blade Runner", "find comedies with Tom Hanks". */
export interface VoiceSearch {
  readonly utterance: string;
  readonly title?: string;
  readonly genre?: string;
  readonly cast?: string;
  readonly contentType?: string;
  readonly releasedFrom?: string;
  readonly releasedTo?: string;
}

/** A transport command: `seconds` is negative backwards, `position` in seconds. */
export type VoiceCommand =
  | {
      readonly type:
        | 'play'
        | 'pause'
        | 'stop'
        | 'restart'
        | 'next'
        | 'previous'
        | 'fastForward'
        | 'rewind';
    }
  | { readonly type: 'skip'; readonly seconds: number }
  | { readonly type: 'seek'; readonly position: number }
  | { readonly type: 'subtitles'; readonly enabled: boolean };

const searchSchema = z.object({
  utterance: z.string(),
  title: z.string().optional(),
  genre: z.string().optional(),
  cast: z.string().optional(),
  contentType: z.string().optional(),
  releasedFrom: z.string().optional(),
  releasedTo: z.string().optional(),
});

const commandSchema = z.discriminatedUnion('type', [
  z.object({
    type: z.enum(['play', 'pause', 'stop', 'restart', 'next', 'previous', 'fastForward', 'rewind']),
  }),
  z.object({ type: z.literal('skip'), seconds: z.number() }),
  z.object({ type: z.literal('seek'), position: z.number().nonnegative() }),
  z.object({ type: z.literal('subtitles'), enabled: z.boolean() }),
]);

/** The search the native host sent, or null when it is malformed. */
export function parseVoiceSearch(payload: unknown): VoiceSearch | null {
  const parsed = searchSchema.safeParse(payload);
  return parsed.success ? parsed.data : null;
}

/** The command the native host sent, or null when it is malformed. */
export function parseVoiceCommand(payload: unknown): VoiceCommand | null {
  const parsed = commandSchema.safeParse(payload);
  return parsed.success ? parsed.data : null;
}
