import { existsSync, mkdtempSync, readFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import type { Codegen } from './codegen.ts';
import { generateLibrarySpecs } from './library-specs.ts';

const SCHEMA = {
  modules: {
    SvgView: {
      type: 'Component',
      components: { RNSVGSvgView: {}, RNSVGPath: { interfaceOnly: true } },
    },
    NativeSvgViewModule: { type: 'NativeModule', moduleName: 'RNSVGSvgViewModule' },
  },
};

let output: string;
const written: { generators: readonly string[]; directory: string }[] = [];

const fakeCodegen: Codegen = {
  schema: () => SCHEMA,
  write: async (generators, _libraryName, _schema, directory) => {
    written.push({ generators, directory });
    return [];
  },
};

beforeEach(() => {
  output = mkdtempSync(join(tmpdir(), 'rn-tizen-specs-'));
  written.length = 0;
});

afterEach(() => {
  rmSync(output, { recursive: true, force: true });
});

describe('generateLibrarySpecs', () => {
  it('lists the components and TurboModules of the schema', async () => {
    const specs = await generateLibrarySpecs(fakeCodegen, 'rnsvg', '/specs', output);

    expect(specs).toEqual({
      components: ['RNSVGSvgView', 'RNSVGPath'],
      turboModules: ['RNSVGSvgViewModule'],
    });
  });

  it('writes components where ReactCommon includes them and module bases at the root', async () => {
    await generateLibrarySpecs(fakeCodegen, 'rnsvg', '/specs', output);

    expect(written.map((call) => call.directory)).toEqual([
      output,
      join(output, 'react', 'renderer', 'components', 'rnsvg'),
    ]);
  });

  it('registers the descriptor of a generated component and not of an interfaceOnly one', async () => {
    await generateLibrarySpecs(fakeCodegen, 'rnsvg', '/specs', output);

    expect(readFileSync(join(output, 'rnsvg-descriptors.cpp'), 'utf8')).toBe(
      [
        '#include <fabric/ComponentDescriptorRegistry.h>',
        '#include <react/renderer/components/rnsvg/ComponentDescriptors.h>',
        '',
        'RN_TIZEN_COMPONENT_DESCRIPTOR(facebook::react::RNSVGSvgViewComponentDescriptor);',
        '',
      ].join('\n'),
    );
  });

  it('writes no registration file when every component is interfaceOnly', async () => {
    const interfaceOnly: Codegen = {
      ...fakeCodegen,
      schema: () => ({
        modules: { Path: { type: 'Component', components: { Path: { interfaceOnly: true } } } },
      }),
    };

    await generateLibrarySpecs(interfaceOnly, 'paths', '/specs', output);

    expect(existsSync(join(output, 'paths-descriptors.cpp'))).toBe(false);
  });
});
