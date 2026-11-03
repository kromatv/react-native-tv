import { describe, expect, it } from 'vitest';
import { tizenPlatformSelectPlugin } from './tizen-platform-select.ts';

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
  tizenPlatformSelectPlugin({ types: { identifier } }).visitor.CallExpression({ node });
  return node.arguments[0]?.properties.map(
    (member) => (member as { key?: { name: string } }).key?.name,
  );
}

describe('tizenPlatformSelectPlugin', () => {
  it('turns the android entry into the tizen one', () => {
    expect(
      run(select([property('ios', 1), property('android', 2), property('default', 3)])),
    ).toEqual(['ios', 'tizen', 'default']);
  });

  it('keeps a tizen entry the app wrote', () => {
    expect(run(select([property('tizen', 1), property('android', 2)]))).toEqual([
      'tizen',
      'android',
    ]);
  });

  it('leaves a select without an android entry to native and default', () => {
    expect(run(select([property('ios', 1), property('native', 2)]))).toEqual(['ios', 'native']);
  });

  it('leaves a select whose keys are not all static', () => {
    const spread = { type: 'SpreadElement', argument: identifier('more') };

    expect(run(select([property('android', 1), spread]))).toEqual(['android', undefined]);
  });

  it('ignores a select on anything but Platform', () => {
    expect(run(select([property('android', 1)], 'Other'))).toEqual(['android']);
  });
});
