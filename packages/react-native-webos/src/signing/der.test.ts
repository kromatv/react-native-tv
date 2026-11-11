import { describe, expect, it } from 'vitest';
import * as der from './der.ts';
import { childrenOf, explicitOf, expectTag, oidOf, readDer, smallIntegerOf } from './der-reader.ts';

describe('der', () => {
  it('writes an integer in its shortest form, with a zero before a high bit', () => {
    expect(der.integer(Buffer.from([0, 0, 0x7f])).toString('hex')).toBe('02017f');
    expect(der.integer(Buffer.from([0x80])).toString('hex')).toBe('02020080');
    expect(der.integer(0).toString('hex')).toBe('020100');
    expect(der.integer(Buffer.alloc(0)).toString('hex')).toBe('020100');
    expect(der.boolean(false).toString('hex')).toBe('010100');
  });

  it('reads back object identifiers, integers and long lengths', () => {
    const value = der.sequence(
      der.oid('1.2.840.113549.1.12.1.6'),
      der.integer(2048),
      der.octets(Buffer.alloc(300)),
    );

    const [oid, iterations, octets] = childrenOf(readDer(value));

    expect(oidOf(oid)).toBe('1.2.840.113549.1.12.1.6');
    expect(smallIntegerOf(iterations)).toBe(2048);
    expect(octets?.contents.length).toBe(300);
  });

  it('refuses a length past the end of the data', () => {
    expect(() => readDer(Buffer.from([0x30, 0x05, 0x00]))).toThrow('past the end');
  });

  it.each([
    ['data that ends mid-value', [0x30], 'ends mid-value'],
    ['a length whose bytes are missing', [0x04, 0x82, 0x01], 'ends mid-value'],
    ['an indefinite length', [0x30, 0x80], 'unsupported length'],
    ['data after the value', [0x05, 0x00, 0x00], 'data after the value'],
  ])('refuses %s', (_, bytes, message) => {
    expect(() => readDer(Buffer.from(bytes))).toThrow(message);
  });

  it('refuses an empty object identifier, an integer too large or negative, and an empty [0]', () => {
    expect(() => oidOf(readDer(Buffer.from([0x06, 0x00])))).toThrow('empty object identifier');
    expect(() => smallIntegerOf(readDer(der.integer(Buffer.alloc(7, 1))))).toThrow('out of range');
    expect(() => smallIntegerOf(readDer(Buffer.from([0x02, 0x01, 0x80])))).toThrow('out of range');
    expect(() => smallIntegerOf(readDer(Buffer.from([0x02, 0x00])))).toThrow('out of range');
    expect(() => explicitOf(readDer(der.explicit(0, Buffer.alloc(0))), 0)).toThrow('empty [0]');
    expect(() => expectTag(undefined, 0x30, 'a sequence')).toThrow('expected a sequence');
  });
});
