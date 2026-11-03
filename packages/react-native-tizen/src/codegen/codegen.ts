import { mkdir, writeFile } from 'node:fs/promises';
import { createRequire } from 'node:module';
import { join } from 'node:path';
import { z } from 'zod';

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
  allGenerators: z.record(z.string(), z.custom<Generator>(isFunction)),
});

const combineModuleSchema = z.object({
  combineSchemasInFileList: z.custom<CombineSchemas>(isFunction),
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
export function loadCodegen(reactNativeDir: string): Codegen {
  const requireFromReactNative = createRequire(join(reactNativeDir, 'package.json'));
  const { allGenerators } = generatorsModuleSchema.parse(
    requireFromReactNative('@react-native/codegen/lib/generators/RNCodegen.js'),
  );
  const { combineSchemasInFileList } = combineModuleSchema.parse(
    requireFromReactNative('@react-native/codegen/lib/cli/combine/combine-js-to-schema.js'),
  );
  return {
    schema: (dirs, libraryName) =>
      combineSchemasInFileList([...dirs], 'android', undefined, libraryName),
    async write(generators, libraryName, schema, directory) {
      await mkdir(directory, { recursive: true });
      const sources: string[] = [];
      for (const name of generators) {
        const generate = allGenerators[name];
        if (generate === undefined) throw new Error(`@react-native/codegen has no ${name}`);
        for (const [fileName, contents] of generate(
          libraryName,
          schema,
          undefined,
          false,
          undefined,
          true,
        )) {
          const path = join(directory, fileName);
          await writeFile(path, contents);
          if (fileName.endsWith('.cpp')) sources.push(path);
        }
      }
      return sources;
    },
  };
}
