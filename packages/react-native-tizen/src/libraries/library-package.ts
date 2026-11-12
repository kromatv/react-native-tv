import { z } from 'zod';

const dependencyMapSchema = z.record(z.string(), z.unknown()).optional();

const peerMetaSchema = z
  .record(z.string(), z.looseObject({ optional: z.boolean().optional() }))
  .optional();

/** The parts of a dependency's package.json that native library discovery reads. */
export const libraryPackageSchema = z.looseObject({
  name: z.string().optional(),
  version: z.string().optional(),
  dependencies: dependencyMapSchema,
  devDependencies: dependencyMapSchema,
  peerDependencies: dependencyMapSchema,
  peerDependenciesMeta: peerMetaSchema,
  codegenConfig: z.unknown().optional(),
  tizen: z.unknown().optional(),
});

export type LibraryPackage = z.infer<typeof libraryPackageSchema>;

/** React Native's `codegenConfig`, as a library's package.json declares it. */
export const codegenConfigSchema = z.object({
  name: z.string().regex(/^[A-Za-z_]\w*$/),
  type: z.enum(['components', 'modules', 'all']),
  jsSrcsDir: z.string().min(1),
});

export type CodegenConfig = z.infer<typeof codegenConfigSchema>;

/** How a package declares its own Tizen implementation (see docs/native-modules.md). */
export const tizenDeclarationSchema = z.object({
  cmake: z.string().min(1),
});

/** The parts of an `expo-module.config.json` that native library discovery reads. */
export const expoModuleConfigSchema = z.looseObject({
  platforms: z.array(z.string()).optional(),
  tizen: z.unknown().optional(),
});

const autolinkingOptionsSchema = z.looseObject({ nativeModulesDir: z.string().optional() });

/** The `expo.autolinking` options of an app's package.json, with its `tizen` overrides. */
export const autolinkingPackageSchema = z.looseObject({
  expo: z
    .looseObject({
      autolinking: autolinkingOptionsSchema
        .extend({ tizen: autolinkingOptionsSchema.optional() })
        .optional(),
    })
    .optional(),
});
