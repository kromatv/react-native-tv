import { describe, expect, it } from 'vitest';
import { attributes, firstTagAttributes } from './xml-attributes.ts';

describe('firstTagAttributes', () => {
  it('reads the first tag of that name, past tags that only start with it', () => {
    const xml = '<manifestx a="0"/><manifest\n  package="p" version="1&amp;2&lt;3&quot;">';

    expect(firstTagAttributes(xml, 'manifest')).toEqual({ package: 'p', version: '1&2<3"' });
    expect(firstTagAttributes('<a\tx="1"><a b="2">', 'a')).toEqual({ x: '1' });
    expect(firstTagAttributes('<a\rb="&apos;&gt;"', 'a')).toBeNull();
    expect(firstTagAttributes('<b x="1">', 'a')).toBeNull();
  });

  it('stops at a value that never closes', () => {
    expect(attributes(' a="1" b="open')).toEqual({ a: '1' });
    expect(attributes(' a="&apos;&gt;"')).toEqual({ a: "'>" });
  });
});
