import { mkdir, writeFile } from 'node:fs/promises';
import { join } from 'node:path';
import { pathToFileURL } from 'node:url';
import { z } from 'zod';
import { requirePackage } from '../app/app-packages.ts';

/** The generators behind a library's TurboModule bases (`<library>JSI.h`). */
export const MODULE_GENERATORS = ['generateModuleH'] as const;

/** The generators behind a library's component props, events, states, shadow nodes and descriptors. */
export const COMPONENT_GENERATORS = [
  'generateComponentDescriptorH',
  'generateComponentDescriptorCpp',
  'generateEventEmitterH',
  'generateEventEmitterCpp',
  'generatePropsH',
  'generatePropsCpp',
  'generateStateH',
  'generateStateCpp',
  'generateShadowNodeH',
  'generateShadowNodeCpp',
] as const;

type Generator = (
  libraryName: string,
  schema: unknown,
  packageName: string | undefined,
  assumeNonnull: boolean,
  headerPrefix: string | undefined,
  includeGetDebugPropsImplementation: boolean,
) => Map<string, string>;

type CombineSchemas = (
  files: string[],
  platform: string,
  exclude: RegExp | undefined,
  libraryName: string | undefined,
) => unknown;

const isFunction = (value: unknown) => typeof value === 'function';

const generatorsModuleSchema = z.object({
  default: z.object({ allGenerators: z.record(z.string(), z.custom<Generator>(isFunction)) }),
});

const combineModuleSchema = z.object({
  default: z.object({ combineSchemasInFileList: z.custom<CombineSchemas>(isFunction) }),
});

/**
 * React Native's codegen, as the app's own react-native ships it. `schema` reads
 * the specs under `dirs` as Android does, since a Tizen bundle falls back to the
 * Android sources: `.android.*` specs count and `.ios.*` do not. `write` runs
 * `generators` into `directory` and returns the `.cpp` files it wrote.
 */
export interface Codegen {
  schema(dirs: readonly string[], libraryName?: string): unknown;
  write(
    generators: readonly string[],
    libraryName: string,
    schema: unknown,
    directory: string,
  ): Promise<string[]>;
}

/** Loads `@react-native/codegen` the way `reactNativeDir` resolves it. */
export async function loadCodegen(reactNativeDir: string): Promise<Codegen> {
  const codegen = requirePackage(reactNativeDir, '@react-native/codegen', 'react-native');
  const load = (file: string): Promise<unknown> =>
    import(pathToFileURL(join(codegen.dir, file)).href);
  const [generatorsModule, combineModule] = await Promise.all([
    load('lib/generators/RNCodegen.js'),
    load('lib/cli/combine/combine-js-to-schema.js'),
  ]);
  const { allGenerators } = generatorsModuleSchema.parse(generatorsModule).default;
  const { combineSchemasInFileList } = combineModuleSchema.parse(combineModule).default;
  return {
    schema: (dirs, libraryName) =>
      combineSchemasInFileList([...dirs], 'android', undefined, libraryName),
    async write(generators, libraryName, schema, directory) {
      await mkdir(directory, { recursive: true });
      const files = generators.flatMap((name) => {
        const generate = allGenerators[name];
        if (generate === undefined) throw new Error(`@react-native/codegen has no ${name}`);
        return [...generate(libraryName, schema, undefined, false, undefined, true)].map(
          ([fileName, contents]) => ({ path: join(directory, fileName), contents }),
        );
      });
      await Promise.all(files.map(({ path, contents }) => writeFile(path, contents)));
      return files.map(({ path }) => path).filter((path) => path.endsWith('.cpp'));
    },
  };
}
