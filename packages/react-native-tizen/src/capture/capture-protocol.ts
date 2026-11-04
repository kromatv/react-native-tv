import { z } from 'zod';
import { printable } from '../logs/log-line.ts';

const MAX_SIDE = 8192;
const MAX_MESSAGE = 4096;
const MAX_ROW_PADDING = 64;

/**
 * A screen a development build captured: `pixels` holds premultiplied BGRA
 * bytes, top row first, rows `stride` bytes apart.
 */
export interface ScreenFrame {
  readonly width: number;
  readonly height: number;
  readonly stride: number;
  readonly pixels: Buffer;
}

const appSchema = z.string().max(256).optional();
const controlHelloSchema = z.object({
  type: z.literal('control'),
  version: z.literal(1),
  app: appSchema,
});
const logHelloSchema = z.object({ type: z.literal('log'), version: z.literal(1), app: appSchema });

const relayRequestSchema = z.object({
  type: z.literal('screenshot-request'),
  device: z.string().max(64).nullable(),
  app: z.string().max(256).nullable().optional(),
});

/** Which connection an app opened, and the package it names: null for a build that names none. */
export type Hello = { readonly kind: 'control' | 'log'; readonly app: string | null };

/** What a relayed screenshot request asks for: the TV and the package, any when null. */
export interface RelayTarget {
  readonly device: string | null;
  readonly app: string | null;
}

const frameHeaderSchema = z
  .object({
    type: z.literal('screenshot'),
    id: z.number().int().nonnegative(),
    width: z.number().int().min(1).max(MAX_SIDE),
    height: z.number().int().min(1).max(MAX_SIDE),
    stride: z.number().int().min(4),
    bytes: z.number().int().min(4),
  })
  .refine((header) => header.stride >= header.width * 4, 'stride is shorter than a row')
  .refine(
    (header) => header.stride <= header.width * 4 + MAX_ROW_PADDING,
    'stride is longer than a padded row',
  )
  .refine(
    (header) => header.bytes === header.stride * header.height,
    'bytes is not stride x height',
  );

const failureSchema = z.object({
  type: z.literal('screenshot-error'),
  id: z.number().int().nonnegative(),
  message: z.string().max(MAX_MESSAGE),
  hint: z.string().max(MAX_MESSAGE).optional(),
});

/** The header a frame's pixels follow. */
export type FrameHeader = z.infer<typeof frameHeaderSchema>;

/** What an app, or a relaying `rn-tizen`, answers a screenshot request with. */
export type CaptureReply =
  | { readonly kind: 'frame'; readonly header: FrameHeader }
  | {
      readonly kind: 'failure';
      readonly id: number;
      readonly message: string;
      readonly hint: string | undefined;
    }
  | { readonly kind: 'invalid'; readonly reason: string };

function parseJson(line: string): unknown {
  try {
    return JSON.parse(line);
  } catch {
    return null;
  }
}

const named = (app: string | undefined): string | null =>
  app === undefined || app === '' ? null : app;

/** The hello an app opens its control link or its log stream with, or null for any other line. */
export function parseHello(line: string): Hello | null {
  const json = parseJson(line);
  const control = controlHelloSchema.safeParse(json);
  if (control.success) return { kind: 'control', app: named(control.data.app) };
  const log = logHelloSchema.safeParse(json);
  return log.success ? { kind: 'log', app: named(log.data.app) } : null;
}

/** What a relayed screenshot request asks for, or undefined when the line is not one. */
export function parseRelayRequest(line: string): RelayTarget | undefined {
  const parsed = relayRequestSchema.safeParse(parseJson(line));
  return parsed.success ? { device: parsed.data.device, app: parsed.data.app ?? null } : undefined;
}

/** Reads a reply line. A header that does not add up is `invalid`, never a frame. */
export function parseCaptureReply(line: string): CaptureReply {
  const json = parseJson(line);
  const failure = failureSchema.safeParse(json);
  if (failure.success) {
    const { id, message, hint } = failure.data;
    return {
      kind: 'failure',
      id,
      message: printable(message),
      hint: hint === undefined ? undefined : printable(hint),
    };
  }
  const header = frameHeaderSchema.safeParse(json);
  if (header.success) return { kind: 'frame', header: header.data };
  return { kind: 'invalid', reason: header.error.issues[0]?.message ?? 'not a reply' };
}

/** How many raw bytes follow `line`: a valid header's `bytes`, else 0. */
export function replyPayloadLength(line: string): number {
  const reply = parseCaptureReply(line);
  return reply.kind === 'frame' ? reply.header.bytes : 0;
}

/** The line that asks an app for its screen. */
export function screenshotRequest(id: number): string {
  return `${JSON.stringify({ type: 'screenshot', id })}\n`;
}

/** The line that turns an app's perf samples on or off. */
export function perfRequest(on: boolean): string {
  return `${JSON.stringify({ type: 'perf', on })}\n`;
}

/** The line that asks a running `rn-tizen run` or `logs` to capture for this process. */
export function relayRequest(device: string | null, app: string | null): string {
  return `${JSON.stringify({ type: 'screenshot-request', device, app })}\n`;
}

/** A frame as bytes on the wire: its header line, then its pixels. */
export function encodeFrame(id: number, frame: ScreenFrame): Buffer {
  const header = {
    type: 'screenshot',
    id,
    width: frame.width,
    height: frame.height,
    stride: frame.stride,
    bytes: frame.pixels.length,
  };
  return Buffer.concat([Buffer.from(`${JSON.stringify(header)}\n`), frame.pixels]);
}

/** A failure as a line on the wire, with what to do about it when known. */
export function encodeFailure(id: number, message: string, hint?: string): string {
  return `${JSON.stringify({ type: 'screenshot-error', id, message, hint })}\n`;
}
