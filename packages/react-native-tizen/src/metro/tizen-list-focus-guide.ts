import { fileURLToPath } from 'node:url';

interface AstNode {
  readonly type: string;
}

interface JSXIdentifier extends AstNode {
  readonly type: 'JSXIdentifier';
  readonly name: string;
}

interface JSXAttribute extends AstNode {
  readonly type: 'JSXAttribute';
  readonly name: AstNode;
}

interface JSXOpeningElement extends AstNode {
  readonly name: AstNode;
  readonly attributes: AstNode[];
}

/** The builders of Babel's `types` {@link tizenListFocusGuidePlugin} uses. */
export interface ListFocusGuideApi {
  readonly types: {
    jsxAttribute(name: AstNode, value: AstNode): AstNode;
    jsxIdentifier(name: string): AstNode;
    jsxExpressionContainer(expression: AstNode): AstNode;
    callExpression(callee: AstNode, args: AstNode[]): AstNode;
    memberExpression(object: AstNode, property: AstNode): AstNode;
    identifier(name: string): AstNode;
    stringLiteral(value: string): AstNode;
    thisExpression(): AstNode;
  };
}

/** A Babel plugin object, as Babel takes it. */
export interface ListFocusGuidePlugin {
  readonly name: string;
  readonly visitor: {
    JSXOpeningElement(
      path: { readonly node: JSXOpeningElement },
      state: { readonly filename?: string | null },
    ): void;
  };
}

/** The react-native-tvos file that wraps every list in a `TVFocusGuideView`. */
export const LIST_FILE = 'virtualized-lists/Lists/VirtualizedList.js';

/** The module the wrapper's style comes from, by the absolute path Metro resolves from any file. */
export const LIST_FOCUS_GUIDE_STYLE_MODULE = fileURLToPath(
  new URL(
    '../../react-native-overrides/Libraries/Lists/list-focus-guide-style.ts',
    import.meta.url,
  ),
);

function isNode<T extends AstNode>(node: AstNode, type: T['type']): node is T {
  return node.type === type;
}

function isListFile(filename: string | null | undefined): boolean {
  return filename?.replaceAll('\\', '/').endsWith(`/${LIST_FILE}`) === true;
}

function isStyle(attribute: AstNode): boolean {
  return (
    isNode<JSXAttribute>(attribute, 'JSXAttribute') &&
    isNode<JSXIdentifier>(attribute.name, 'JSXIdentifier') &&
    attribute.name.name === 'style'
  );
}

function styleAttribute(types: ListFocusGuideApi['types']): AstNode {
  const helper = types.memberExpression(
    types.callExpression(types.identifier('require'), [
      types.stringLiteral(LIST_FOCUS_GUIDE_STYLE_MODULE),
    ]),
    types.identifier('listFocusGuideStyle'),
  );
  const listStyle = types.memberExpression(
    types.memberExpression(types.thisExpression(), types.identifier('props')),
    types.identifier('style'),
  );
  return types.jsxAttribute(
    types.jsxIdentifier('style'),
    types.jsxExpressionContainer(types.callExpression(helper, [listStyle])),
  );
}

/**
 * For a Tizen bundle: gives the `TVFocusGuideView` react-native-tvos wraps a
 * list in the flex its ScrollView takes, from `listFocusGuideStyle`. The
 * wrapper has no flex and a 1 px minimum size on every TV platform, so a list
 * sized only by `flex: 1` collapsed to 1 px.
 */
export function tizenListFocusGuidePlugin({ types }: ListFocusGuideApi): ListFocusGuidePlugin {
  return {
    name: 'rn-tizen-list-focus-guide',
    visitor: {
      JSXOpeningElement(path, state) {
        if (!isListFile(state.filename)) return;
        const { node } = path;
        if (!isNode<JSXIdentifier>(node.name, 'JSXIdentifier')) return;
        if (node.name.name !== 'TVFocusGuideView' || node.attributes.some(isStyle)) return;
        node.attributes.push(styleAttribute(types));
      },
    },
  };
}
