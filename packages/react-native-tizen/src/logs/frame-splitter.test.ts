import { describe, expect, it } from 'vitest';
import { splitFrames, type StreamFrame } from './frame-splitter.ts';

function collect(payloadLength: (line: string) => number = () => 0, maxLineBytes = 1024) {
  const frames: StreamFrame[] = [];
  const sink = splitFrames({ maxLineBytes, payloadLength, onFrame: (frame) => frames.push(frame) });
  return { frames, sink };
}

describe('splitFrames', () => {
  it('splits lines across chunks and skips blank ones', () => {
    const { frames, sink } = collect();

    sink.push(Buffer.from('one\n\ntw'));
    sink.push(Buffer.from('o\nthree'));
    sink.end();

    expect(frames.map((frame) => frame.line)).toEqual(['one', 'two', 'three']);
  });

  it('delivers the bytes a line announced with it, whatever they contain', () => {
    const { frames, sink } = collect((line) => (line === 'header' ? 4 : 0));

    sink.push(Buffer.from('header\n\n\x00'));
    sink.push(Buffer.from('\nzafter\n'));
    sink.end();

    expect(frames).toEqual([
      { line: 'header', payload: Buffer.from('\n\x00\nz') },
      { line: 'after', payload: null },
    ]);
  });

  it('cuts a line longer than the limit', () => {
    const { frames, sink } = collect(() => 0, 4);

    sink.push(Buffer.from('abcdefgh'));

    expect(frames.map((frame) => frame.line)).toEqual(['abcd']);
  });

  it('drops a payload the stream ended before', () => {
    const { frames, sink } = collect(() => 10);

    sink.push(Buffer.from('header\nabc'));
    sink.end();

    expect(frames).toEqual([]);
  });
});
