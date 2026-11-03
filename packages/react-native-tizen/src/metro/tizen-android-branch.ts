interface AstNode {
  readonly type: string;
}

interface Identifier extends AstNode {
  readonly type: 'Identifier';
  readonly name: string;
}

interface MemberExpression extends AstNode {
  readonly type: 'MemberExpression';
  readonly object: AstNode;
  readonly property: AstNode;
  readonly computed?: boolean;
}

interface MemberPath {
  readonly node: MemberExpression;
  readonly parent: AstNode & { readonly left?: AstNode };
  replaceWith(node: AstNode): void;
}

/** The part of Babel's plugin API {@link tizenAndroidBranchPlugin} uses. */
export interface AndroidBranchApi {
  readonly types: { stringLiteral(value: string): AstNode };
}

/** A Babel plugin object, as Babel takes it. */
export interface AndroidBranchPlugin {
  readonly name: string;
  readonly visitor: {
    MemberExpression(path: MemberPath, state: { readonly filename?: string | null }): void;
  };
}

/**
 * React Native files that choose a whole component by `Platform.OS` and take
 * neither branch on `tizen`: TextInput renders nothing and never focuses.
 * Tizen draws Android's `AndroidTextInput`, so these read `android`.
 */
export const ANDROID_BRANCH_FILES = [
  'Libraries/Components/TextInput/TextInput.js',
  'Libraries/Components/TextInput/TextInputState.js',
] as const;

function takesAndroidBranch(filename: string | null | undefined): boolean {
  if (!filename) return false;
  const path = filename.replaceAll('\\', '/');
  return ANDROID_BRANCH_FILES.some((file) => path.endsWith(`/${file}`));
}

function isIdentifier(node: AstNode, name: string): boolean {
  return node.type === 'Identifier' && (node as Identifier).name === name;
}

function isPlatformOs(node: MemberExpression): boolean {
  return (
    node.computed !== true &&
    isIdentifier(node.object, 'Platform') &&
    isIdentifier(node.property, 'OS')
  );
}

/**
 * For a Tizen bundle: inside {@link ANDROID_BRANCH_FILES}, `Platform.OS` is
 * the string `'android'`, so each file takes its Android path, as the files
 * this package overrides do. It runs before Expo inlines `Platform.OS` as
 * `'tizen'` in a production build, since a plugin runs before a preset.
 */
export function tizenAndroidBranchPlugin({ types }: AndroidBranchApi): AndroidBranchPlugin {
  return {
    name: 'rn-tizen-android-branch',
    visitor: {
      MemberExpression(path, state) {
        if (!takesAndroidBranch(state.filename) || !isPlatformOs(path.node)) return;
        if (path.parent.type === 'AssignmentExpression' && path.parent.left === path.node) return;
        path.replaceWith(types.stringLiteral('android'));
      },
    },
  };
}
