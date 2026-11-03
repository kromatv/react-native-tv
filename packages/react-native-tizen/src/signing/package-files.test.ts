import { describe, expect, it } from 'vitest';
import { isSignatureFile, referenceUri, signedFiles } from './package-files.ts';

describe('referenceUri', () => {
  it('escapes the slash, reserved characters and UTF-8 as Tizen Studio does', () => {
    expect(referenceUri('res/a b+c&d.txt')).toBe('res%2Fa%20b%2Bc%26d.txt');
    expect(referenceUri('res/ünï.txt')).toBe('res%2F%C3%BCn%C3%AF.txt');
    expect(referenceUri('res/100%.txt')).toBe('res%2F100%25.txt');
    expect(referenceUri('res/semi;colon=eq@at.txt')).toBe('res%2Fsemi%3Bcolon%3Deq%40at.txt');
    expect(referenceUri('a"b')).toBe('a&quot;b');
  });
});

describe('signedFiles', () => {
  const files = new Map([
    ['tizen-manifest.xml', Buffer.from('m')],
    ['author-signature.xml', Buffer.from('a')],
    ['signature1.xml', Buffer.from('d')],
    ['res/signature1.xml', Buffer.from('n')],
    ['Bin.dll', Buffer.from('b')],
  ]);

  it('leaves the author signature out of itself, and every distributor signature out of both', () => {
    expect(signedFiles(files, 'author').map((file) => file.uri)).toEqual([
      'Bin.dll',
      'res%2Fsignature1.xml',
      'tizen-manifest.xml',
    ]);
    expect(signedFiles(files, 'distributor').map((file) => file.uri)).toEqual([
      'Bin.dll',
      'author-signature.xml',
      'res%2Fsignature1.xml',
      'tizen-manifest.xml',
    ]);
  });

  it('counts only root files named like signatures as signatures', () => {
    expect(isSignatureFile('signature12.xml')).toBe(true);
    expect(isSignatureFile('res/signature1.xml')).toBe(false);
    expect(isSignatureFile('signature0.xml')).toBe(false);
  });
});
