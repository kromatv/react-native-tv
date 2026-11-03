import { describe, expect, it } from 'vitest';
import { parsePreviewSource } from './preview-source.ts';

describe('parsePreviewSource', () => {
  it('takes an https url with headers', () => {
    const source = { url: 'https://example.com/me.json', headers: { Authorization: 'Bearer x' } };

    expect(parsePreviewSource(source)).toEqual(source);
  });

  it('takes an http url without headers', () => {
    expect(parsePreviewSource({ url: 'http://192.168.1.50/preview.json' })).toEqual({
      url: 'http://192.168.1.50/preview.json',
    });
  });

  it('refuses headers to an http url', () => {
    const source = { url: 'http://example.com/me.json', headers: { Authorization: 'Bearer x' } };

    expect(() => parsePreviewSource(source)).toThrow('headers go to an https url only');
  });

  it('refuses a url that is not http or https', () => {
    expect(() => parsePreviewSource({ url: 'file:///etc/passwd' })).toThrow();
  });
});
