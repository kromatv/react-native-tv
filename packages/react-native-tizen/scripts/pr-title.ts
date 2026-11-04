import { z } from 'zod';

const ReleasePleaseConfig = z.object({
  packages: z.record(
    z.string(),
    z.object({ 'changelog-sections': z.array(z.object({ type: z.string() })) }),
  ),
});

/** The commit types release-please-config.json gives a changelog section, shown or hidden. */
export function changelogTypes(configJson: string): string[] {
  const config = ReleasePleaseConfig.parse(JSON.parse(configJson));
  const sections = Object.values(config.packages).flatMap((pkg) => pkg['changelog-sections']);
  return [...new Set(sections.map((section) => section.type))];
}

const CONVENTIONAL = /^(?<type>[a-z]+)(?:\([a-z0-9-]+(?:,[a-z0-9-]+)*\))?!?: \S/;

/** Why `title` cannot become a squash commit Release Please reads, or undefined when it can. */
export function prTitleProblem(title: string, types: readonly string[]): string | undefined {
  const type = CONVENTIONAL.exec(title)?.groups?.type;
  if (type === undefined) {
    return 'The title is not in the form type(scope): summary';
  }
  if (!types.includes(type)) return `"${type}" is not a commit type the changelog knows`;
  return undefined;
}
