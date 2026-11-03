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

/** The part of Babel's plugin API {@link tizenPlatformSelectPlugin} uses. */
export interface BabelApi {
  readonly types: { identifier(name: string): AstNode };
}

/** A Babel plugin object, as Babel takes it. */
export interface BabelPlugin {
  readonly name: string;
  readonly visitor: { CallExpression(path: { readonly node: CallExpression }): void };
}

function isIdentifier(node: AstNode, name: string): boolean {
  return node.type === 'Identifier' && (node as Identifier).name === name;
}

function isPlatformSelect(callee: AstNode): boolean {
  if (callee.type !== 'MemberExpression') return false;
  const member = callee as MemberExpression;
  return (
    member.computed !== true &&
    isIdentifier(member.object, 'Platform') &&
    isIdentifier(member.property, 'select')
  );
}

function staticKey(member: ObjectMember): string | null {
  if (member.computed === true) return null;
  if (member.type === 'ObjectMethod' && member.kind !== 'method') return null;
  if (member.type !== 'ObjectProperty' && member.type !== 'ObjectMethod') return null;
  if (member.key.type === 'Identifier') return (member.key as Identifier).name;
  if (member.key.type === 'StringLiteral') return (member.key as StringLiteral).value;
  return null;
}

/**
 * For a Tizen bundle: in a `Platform.select({...})` whose keys are all static
 * and that has no `tizen` entry, the `android` entry becomes the `tizen` one.
 * Expo inlines `Platform.select` at build time with the bundle's platform,
 * then `native`, then `default`; after this, the inlined value is the one
 * `Platform.select` picks at run time on Tizen: `tizen`, then `android`.
 */
export function tizenPlatformSelectPlugin({ types }: BabelApi): BabelPlugin {
  return {
    name: 'rn-tizen-platform-select',
    visitor: {
      CallExpression(path) {
        const [spec] = path.node.arguments;
        if (!isPlatformSelect(path.node.callee) || spec?.type !== 'ObjectExpression') return;
        const keys = (spec as ObjectExpression).properties.map(staticKey);
        if (keys.includes(null) || keys.includes('tizen')) return;
        const android = keys.indexOf('android');
        const member = (spec as ObjectExpression).properties[android];
        if (member !== undefined) member.key = types.identifier('tizen');
      },
    },
  };
}
