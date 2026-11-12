import { describe, expect, it } from 'vitest';
import { jsonObjectAt, jsonObjectIn, whitespaceBefore } from './json-object.ts';

const TEXT = `{
  "name": "a \\"quoted\\" } name",
  "list": [1, { "in": "[" }, "]"],
  "nested": { "deep": { "leaf": true }, "n": -1.5e3 },
  "empty": {}
}
`;

describe('jsonObjectAt', () => {
  it('gives each member of the object with where its key and value sit', () => {
    const object = jsonObjectAt(TEXT, 0);
    const slices = object?.members.map((member) => [
      member.key,
      TEXT.slice(member.keyStart, member.keyEnd),
      TEXT.slice(member.valueStart, member.valueEnd),
    ]);

    expect(slices).toEqual([
      ['name', '"name"', '"a \\"quoted\\" } name"'],
      ['list', '"list"', '[1, { "in": "[" }, "]"]'],
      ['nested', '"nested"', '{ "deep": { "leaf": true }, "n": -1.5e3 }'],
      ['empty', '"empty"', '{}'],
    ]);
    expect(TEXT.slice(object?.start, object?.end)).toBe(TEXT.trimEnd());
  });

  it('reads an object with no whitespace, and one with none of its own', () => {
    expect(jsonObjectAt('{"a":1,"b":{"c":null}}', 0)?.members.map((member) => member.key)).toEqual([
      'a',
      'b',
    ]);
    expect(jsonObjectAt('  {}', 0)).toEqual({ start: 2, end: 4, members: [] });
  });

  it('gives null where a value of another kind is', () => {
    expect(jsonObjectAt('[{}]', 0)).toBeNull();
    expect(jsonObjectAt('"{"', 0)).toBeNull();
    expect(jsonObjectAt('', 0)).toBeNull();
  });

  it('stops at the end of text that is cut short', () => {
    expect(jsonObjectAt('{"a": [1, "b', 0)?.members).toHaveLength(1);
    expect(jsonObjectAt('{"a": tru', 0)?.members).toHaveLength(1);
  });
});

describe('jsonObjectIn', () => {
  it('finds an object by the keys that lead to it, the last of two keys winning', () => {
    const deep = jsonObjectIn(TEXT, ['nested', 'deep']);

    expect(TEXT.slice(deep?.start, deep?.end)).toBe('{ "leaf": true }');
    expect(jsonObjectIn('{"a": {"x": 1}, "a": {"y": 2}}', ['a'])?.members[0]?.key).toBe('y');
  });

  it('gives null for a key that is not there or holds no object', () => {
    expect(jsonObjectIn(TEXT, ['missing'])).toBeNull();
    expect(jsonObjectIn(TEXT, ['name'])).toBeNull();
    expect(jsonObjectIn(TEXT, ['name', 'deeper'])).toBeNull();
  });
});

describe('whitespaceBefore', () => {
  it('gives the whitespace that ends at an offset', () => {
    expect(whitespaceBefore('{\r\n\t"a": 1}', 4)).toBe('\r\n\t');
    expect(whitespaceBefore('{"a": 1}', 1)).toBe('');
    expect(whitespaceBefore('  x', 2)).toBe('  ');
  });
});
