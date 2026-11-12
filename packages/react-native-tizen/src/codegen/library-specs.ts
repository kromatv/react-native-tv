import { mkdir, rm, writeFile } from 'node:fs/promises';
import { join } from 'node:path';
import { z } from 'zod';
import { type Codegen, COMPONENT_GENERATORS, MODULE_GENERATORS } from './codegen.ts';

const componentSchema = z.looseObject({ interfaceOnly: z.boolean().optional() });

const specSchema = z.looseObject({
  type: z.string(),
  moduleName: z.string().optional(),
  components: z.record(z.string(), componentSchema).optional(),
});

const librarySchema = z.looseObject({ modules: z.record(z.string(), specSchema) });

/** What codegen produced for one library. */
export interface LibrarySpecs {
  readonly components: readonly string[];
  readonly turboModules: readonly string[];
}

function descriptorRegistrations(libraryName: string, components: readonly string[]): string {
  const lines = components.map(
    (component) =>
      `RN_TIZEN_COMPONENT_DESCRIPTOR(facebook::react::${component}ComponentDescriptor);`,
  );
  return [
    '#include <fabric/ComponentDescriptorRegistry.h>',
    `#include <react/renderer/components/${libraryName}/ComponentDescriptors.h>`,
    '',
    ...lines,
    '',
  ].join('\n');
}

/**
 * Runs codegen over one library's specs into `outputDir`, C++ only: the
 * TurboModule bases as `<libraryName>JSI.h` at its root, the components under
 * `react/renderer/components/<libraryName>/`, and a file registering the
 * descriptor of every component codegen writes a shadow node for. Components
 * declared `interfaceOnly` get no registration: the library's own C++ owns
 * their shadow nodes.
 */
export async function generateLibrarySpecs(
  codegen: Codegen,
  libraryName: string,
  specsDir: string,
  outputDir: string,
): Promise<LibrarySpecs> {
  const schema = codegen.schema([specsDir], libraryName);
  const { modules } = librarySchema.parse(schema);
  const specs = Object.values(modules);
  const components = specs.flatMap((spec) => Object.entries(spec.components ?? {}));
  const generated = components
    .filter(([, component]) => component.interfaceOnly !== true)
    .map(([name]) => name);

  await rm(outputDir, { recursive: true, force: true });
  await mkdir(outputDir, { recursive: true });
  await codegen.write(MODULE_GENERATORS, libraryName, schema, outputDir);
  await codegen.write(
    COMPONENT_GENERATORS,
    libraryName,
    schema,
    join(outputDir, 'react', 'renderer', 'components', libraryName),
  );
  if (generated.length > 0) {
    await writeFile(
      join(outputDir, `${libraryName}-descriptors.cpp`),
      descriptorRegistrations(libraryName, generated),
    );
  }
  return {
    components: components.map(([name]) => name),
    turboModules: specs.flatMap((spec) => (spec.moduleName === undefined ? [] : [spec.moduleName])),
  };
}
