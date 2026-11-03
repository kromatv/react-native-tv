import { rm } from 'node:fs/promises';
import { join } from 'node:path';
import { COMPONENT_GENERATORS, loadCodegen, MODULE_GENERATORS } from './codegen.ts';

const LIBRARY_NAME = 'FBReactNativeSpec';

interface GeneratedSpecs {
  readonly includeDir: string;
  readonly sources: readonly string[];
}

/**
 * Runs React Native's own codegen over its specs and writes the C++ half only:
 * the TurboModule bases (`FBReactNativeSpec/FBReactNativeSpecJSI.h`) and the core
 * components' props, events, states, shadow nodes and descriptors, at the include
 * paths ReactCommon expects. The schema is Android's, because the JavaScript a
 * Tizen bundle runs falls back to the Android sources.
 */
export async function generateReactNativeSpecs(
  reactNativeDir: string,
  outputDir: string,
): Promise<GeneratedSpecs> {
  const codegen = loadCodegen(reactNativeDir);
  const schema = codegen.schema([join(reactNativeDir, 'src')]);

  await rm(outputDir, { recursive: true, force: true });
  const moduleSources = await codegen.write(
    MODULE_GENERATORS,
    LIBRARY_NAME,
    schema,
    join(outputDir, LIBRARY_NAME),
  );
  const componentSources = await codegen.write(
    COMPONENT_GENERATORS,
    LIBRARY_NAME,
    schema,
    join(outputDir, 'react', 'renderer', 'components', LIBRARY_NAME),
  );
  return { includeDir: outputDir, sources: [...moduleSources, ...componentSources] };
}
