import { describe, expect, it } from 'vitest';
import {
  encodeFailure,
  encodeFrame,
  isControlHello,
  parseCaptureReply,
  relayRequestDevice,
  replyPayloadLength,
} from './capture-protocol.ts';

describe('parseCaptureReply', () => {
  it('reads a header whose size adds up', () => {
    const wire = encodeFrame(7, { width: 2, height: 3, stride: 8, pixels: Buffer.alloc(24) });
    const line = wire.toString('utf8', 0, wire.indexOf(0x0a));

    expect(parseCaptureReply(line)).toEqual({
      kind: 'frame',
      header: { type: 'screenshot', id: 7, width: 2, height: 3, stride: 8, bytes: 24 },
    });
    expect(replyPayloadLength(line)).toBe(24);
  });

  it('refuses a header whose bytes do not match its size, so nothing is read for it', () => {
    const line = JSON.stringify({
      type: 'screenshot',
      id: 1,
      width: 2,
      height: 2,
      stride: 8,
      bytes: 1e9,
    });

    expect(parseCaptureReply(line).kind).toBe('invalid');
    expect(replyPayloadLength(line)).toBe(0);
  });

  it('refuses a stride shorter than a row', () => {
    const line = JSON.stringify({
      type: 'screenshot',
      id: 1,
      width: 4,
      height: 1,
      stride: 8,
      bytes: 8,
    });

    expect(parseCaptureReply(line).kind).toBe('invalid');
  });

  it('refuses a stride longer than a row and its padding', () => {
    const line = JSON.stringify({
      type: 'screenshot',
      id: 1,
      width: 2,
      height: 1,
      stride: 8 + 65,
      bytes: 8 + 65,
    });

    expect(parseCaptureReply(line)).toEqual({
      kind: 'invalid',
      reason: 'stride is longer than a padded row',
    });
  });

  it('reads a stride with up to 64 bytes of row padding', () => {
    const line = JSON.stringify({
      type: 'screenshot',
      id: 1,
      width: 2,
      height: 1,
      stride: 8 + 64,
      bytes: 8 + 64,
    });

    expect(parseCaptureReply(line).kind).toBe('frame');
  });

  it('strips control characters from a failure the app sends', () => {
    const line = encodeFailure(3, 'no \u001b[2Jframe', 'try\u0007 later').trim();

    expect(parseCaptureReply(line)).toEqual({
      kind: 'failure',
      id: 3,
      message: 'no [2Jframe',
      hint: 'try later',
    });
  });

  it('reads a failure with its hint', () => {
    expect(parseCaptureReply(encodeFailure(3, 'no frame', 'try later').trim())).toEqual({
      kind: 'failure',
      id: 3,
      message: 'no frame',
      hint: 'try later',
    });
  });
});

describe('connection roles', () => {
  it('knows the control hello and nothing else as one', () => {
    expect(isControlHello('{"type":"control","version":1}')).toBe(true);
    expect(isControlHello('{"level":"info","message":"control"}')).toBe(false);
    expect(isControlHello('not json')).toBe(false);
  });

  it('reads the device a relay request names, null for any', () => {
    expect(relayRequestDevice('{"type":"screenshot-request","device":"192.168.1.50"}')).toBe(
      '192.168.1.50',
    );
    expect(relayRequestDevice('{"type":"screenshot-request","device":null}')).toBeNull();
    expect(relayRequestDevice('{"level":"info","message":"hi"}')).toBeUndefined();
  });
});
