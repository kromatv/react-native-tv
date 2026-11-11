interface AstNode {
  readonly type: string;
}

interface Identifier extends AstNode {
  readonly type: 'Identifier';
  readonly name: string;
}

interface StringLiteral extends AstNode {
  readonly type: 'StringLiteral';
  readonly value: string;
}

interface ObjectMember extends AstNode {
  key: AstNode;
  readonly computed?: boolean;
  readonly kind?: string;
}

interface ObjectExpression extends AstNode {
  readonly type: 'ObjectExpression';
  readonly properties: readonly ObjectMember[];
}

interface MemberExpression extends AstNode {
  readonly type: 'MemberExpression';
  readonly object: AstNode;
  readonly property: AstNode;
  readonly computed?: boolean;
}

interface CallExpression extends AstNode {
  readonly callee: AstNode;
  readonly arguments: readonly AstNode[];
}

/** The part of Babel's plugin API {@link webosPlatformSelectPlugin} uses. */
export interface BabelApi {
  readonly types: { identifier(name: string): AstNode };
}

/** A Babel plugin object, as Babel takes it. */
export interface BabelPlugin {
  readonly name: string;
  readonly visitor: { CallExpression(path: { readonly node: CallExpression }): void };
}

function isNode<T extends AstNode>(node: AstNode, type: T['type']): node is T {
  return node.type === type;
}

function isIdentifier(node: AstNode, name: string): boolean {
  return isNode<Identifier>(node, 'Identifier') && node.name === name;
}

function isPlatformSelect(callee: AstNode): boolean {
  return (
    isNode<MemberExpression>(callee, 'MemberExpression') &&
    callee.computed !== true &&
    isIdentifier(callee.object, 'Platform') &&
    isIdentifier(callee.property, 'select')
  );
}

function staticKey(member: ObjectMember): string | null {
  if (member.computed === true) return null;
  if (member.type === 'ObjectMethod' && member.kind !== 'method') return null;
  if (member.type !== 'ObjectProperty' && member.type !== 'ObjectMethod') return null;
  if (isNode<Identifier>(member.key, 'Identifier')) return member.key.name;
  if (isNode<StringLiteral>(member.key, 'StringLiteral')) return member.key.value;
  return null;
}

/**
 * For a webOS bundle: in a `Platform.select({...})` whose keys are all static
 * and that has no `webos` entry, the `android` entry becomes the `webos` one.
 * Expo inlines `Platform.select` at build time with the bundle's platform,
 * then `native`, then `default`; after this, the inlined value is the one
 * `Platform.select` picks at run time on webOS: `webos`, then `android`.
 */
export function webosPlatformSelectPlugin({ types }: BabelApi): BabelPlugin {
  return {
    name: 'rn-webos-platform-select',
    visitor: {
      CallExpression(path) {
        const [spec] = path.node.arguments;
        if (!isPlatformSelect(path.node.callee) || spec === undefined) return;
        if (!isNode<ObjectExpression>(spec, 'ObjectExpression')) return;
        const keys = spec.properties.map(staticKey);
        if (keys.includes(null) || keys.includes('webos')) return;
        const member = spec.properties[keys.indexOf('android')];
        if (member !== undefined) member.key = types.identifier('webos');
      },
    },
  };
}
