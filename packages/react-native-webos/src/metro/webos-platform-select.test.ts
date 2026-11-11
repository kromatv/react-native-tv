import { describe, expect, it } from 'vitest';
import { webosPlatformSelectPlugin } from './webos-platform-select.ts';

const identifier = (name: string) => ({ type: 'Identifier', name });
const property = (key: string, value: number) => ({
  type: 'ObjectProperty',
  key: identifier(key),
  value: { type: 'NumericLiteral', value },
});
const select = (properties: object[], object = 'Platform') => ({
  type: 'CallExpression',
  callee: { type: 'MemberExpression', object: identifier(object), property: identifier('select') },
  arguments: [{ type: 'ObjectExpression', properties }],
});

function run(node: ReturnType<typeof select>) {
  webosPlatformSelectPlugin({ types: { identifier } }).visitor.CallExpression({ node });
  return node.arguments[0]?.properties.map(
    (member) => (member as { key?: { name: string } }).key?.name,
  );
}

describe('webosPlatformSelectPlugin', () => {
  it('turns the android entry into the webos one', () => {
    expect(
      run(select([property('ios', 1), property('android', 2), property('default', 3)])),
    ).toEqual(['ios', 'webos', 'default']);
  });

  it('keeps a webos entry the app wrote', () => {
    expect(run(select([property('webos', 1), property('android', 2)]))).toEqual([
      'webos',
      'android',
    ]);
  });

  it('leaves a select without an android entry to native and default', () => {
    expect(run(select([property('ios', 1), property('native', 2)]))).toEqual(['ios', 'native']);
  });

  it("reads a key written as a string, and turns 'android' into webos", () => {
    const quoted = { ...property('x', 1), key: { type: 'StringLiteral', value: 'android' } };

    expect(run(select([property('ios', 1), quoted]))).toEqual(['ios', 'webos']);
  });

  it('leaves a select with a numeric key', () => {
    const numeric = { ...property('x', 1), key: { type: 'NumericLiteral', value: 0 } };

    expect(run(select([property('android', 1), numeric]))).toEqual(['android', undefined]);
  });

  it('leaves a select whose keys are not all static', () => {
    const spread = { type: 'SpreadElement', argument: identifier('more') };

    expect(run(select([property('android', 1), spread]))).toEqual(['android', undefined]);
  });

  it('ignores a select on anything but Platform', () => {
    expect(run(select([property('android', 1)], 'Other'))).toEqual(['android']);
  });
});
