import { describe, expect, it } from 'vitest';
import { tizenAndroidBranchPlugin } from './tizen-android-branch.ts';

const identifier = (name: string) => ({ type: 'Identifier', name });
const member = (object: string, property: string) => ({
  type: 'MemberExpression' as const,
  object: identifier(object),
  property: identifier(property),
});

const TEXT_INPUT = '/app/node_modules/react-native/Libraries/Components/TextInput/TextInput.js';

function run(
  node: ReturnType<typeof member>,
  filename: string,
  parent: object = { type: 'BinaryExpression' },
) {
  let replaced: unknown = node;
  const plugin = tizenAndroidBranchPlugin({
    types: { stringLiteral: (value: string) => ({ type: 'StringLiteral', value }) },
  });
  plugin.visitor.MemberExpression(
    { node, parent: parent as { type: string }, replaceWith: (next) => (replaced = next) },
    { filename },
  );
  return replaced;
}

describe('tizenAndroidBranchPlugin', () => {
  it('reads Platform.OS as android in TextInput', () => {
    expect(run(member('Platform', 'OS'), TEXT_INPUT)).toEqual({
      type: 'StringLiteral',
      value: 'android',
    });
  });

  it('reads Platform.OS as android in TextInputState, on a Windows path too', () => {
    const filename =
      'C:\\app\\node_modules\\react-native\\Libraries\\Components\\TextInput\\TextInputState.js';

    expect(run(member('Platform', 'OS'), filename)).toEqual({
      type: 'StringLiteral',
      value: 'android',
    });
  });

  it('leaves Platform.OS alone in every other file', () => {
    const node = member('Platform', 'OS');

    expect(run(node, '/app/node_modules/react-native/Libraries/Alert/Alert.js')).toBe(node);
  });

  it('leaves another member of Platform alone', () => {
    const node = member('Platform', 'isTV');

    expect(run(node, TEXT_INPUT)).toBe(node);
  });

  it('leaves an assignment to Platform.OS alone', () => {
    const node = member('Platform', 'OS');

    expect(run(node, TEXT_INPUT, { type: 'AssignmentExpression', left: node })).toBe(node);
  });
});
