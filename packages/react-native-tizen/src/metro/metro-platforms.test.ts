import { rmSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { appDir } from '../app/app-dir.fixtures.ts';
import { FixableError } from '../errors.ts';
import { metroPlatforms, tizenMetroIssue } from './metro-platforms.ts';

const dirs: string[] = [];

afterEach(() => {
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

function app(metroConfig: string | null): string {
  const dir = appDir({ name: 'Metro', slug: 'metro' });
  dirs.push(dir);
  if (metroConfig !== null) writeFileSync(join(dir, 'metro.config.mjs'), metroConfig);
  return dir;
}

describe('metroPlatforms', () => {
  it("loads an ES module Metro config as expo start does, with Expo's own platforms", () => {
    const dir = app("export default { resolver: { platforms: ['ios', 'android', 'tizen'] } };\n");

    expect(metroPlatforms(dir)).toEqual(expect.arrayContaining(['ios', 'android', 'tizen']));
    expect(tizenMetroIssue(dir)).toBeNull();
  });

  it('names rn-tizen init when the config does not know tizen, or there is none', () => {
    const issue = tizenMetroIssue(app(null));

    expect(issue).toBeInstanceOf(FixableError);
    expect(issue?.message).toContain('does not know the tizen platform');
    expect(issue?.hint).toContain('npx rn-tizen init');
  });

  it('says why when Expo cannot load the config', () => {
    const dir = app("throw new Error('broken metro config');\n");

    expect(() => metroPlatforms(dir)).toThrow(
      "Could not load the app's Metro config: broken metro config",
    );
  });
});
