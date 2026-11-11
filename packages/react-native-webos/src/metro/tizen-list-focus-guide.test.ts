import { readFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, join } from 'node:path';
import { describe, expect, it } from 'vitest';
import { z } from 'zod';
import { PACKAGE_ROOT } from '../package-paths.ts';
import {
  LIST_FILE,
  LIST_FOCUS_GUIDE_STYLE_MODULE,
  tizenListFocusGuidePlugin,
} from './tizen-list-focus-guide.ts';

const fromExample = createRequire(join(PACKAGE_ROOT, 'example', 'package.json'));
const babel = z
  .object({
    transformSync: z.custom<(code: string, options: object) => { code?: string | null } | null>(
      (value) => typeof value === 'function',
    ),
  })
  .parse(fromExample('@babel/core'));
const hermesParser = createRequire(fromExample.resolve('babel-preset-expo')).resolve(
  'babel-plugin-syntax-hermes-parser',
);
const virtualizedList = join(
  dirname(
    createRequire(fromExample.resolve('react-native/package.json')).resolve(
      '@react-native-tvos/virtualized-lists/package.json',
    ),
  ),
  'Lists',
  'VirtualizedList.js',
);

function transform(source: string, filename: string): string {
  const result = babel.transformSync(source, {
    babelrc: false,
    configFile: false,
    filename,
    plugins: [hermesParser, tizenListFocusGuidePlugin],
  });
  return result?.code ?? '';
}

describe('tizenListFocusGuidePlugin', () => {
  it("gives react-native-tvos's list wrapper the list's flex", () => {
    const code = transform(readFileSync(virtualizedList, 'utf8'), virtualizedList);

    expect(virtualizedList.replaceAll('\\', '/').endsWith(LIST_FILE)).toBe(true);
    expect(code).toContain(
      `style={require(${JSON.stringify(LIST_FOCUS_GUIDE_STYLE_MODULE)}).listFocusGuideStyle(this.props.style)}`,
    );
    expect(code.match(/<TVFocusGuideView\b/g)).toHaveLength(1);
  });

  it('keeps a style the wrapper already has', () => {
    const source = 'const a = <TVFocusGuideView style={own} trapFocusUp />;';

    expect(transform(source, `/app/node_modules/@react-native-tvos/${LIST_FILE}`)).toBe(source);
  });

  it('leaves every other file and every other element alone', () => {
    const guide = 'const a = <TVFocusGuideView trapFocusUp />;';
    const view = 'const b = <View trapFocusUp />;';

    expect(transform(guide, '/app/src/Rail.js')).toBe(guide);
    expect(transform(view, `/app/node_modules/@react-native-tvos/${LIST_FILE}`)).toBe(view);
    expect(transform('const c = <Lists.Guide />;', `/app/${LIST_FILE}`)).toBe(
      'const c = <Lists.Guide />;',
    );
  });
});
