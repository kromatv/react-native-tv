import { readFileSync } from 'node:fs';
import { describe, expect, it } from 'vitest';
import { changelogTypes, prTitleProblem } from './pr-title.ts';

const types = changelogTypes(readFileSync('release-please-config.json', 'utf8'));

describe('changelogTypes', () => {
  it('reads every type the changelog has a section for, shown or hidden', () => {
    expect(types).toEqual(expect.arrayContaining(['feat', 'fix', 'docs', 'chore', 'ci', 'test']));
  });

  it('refuses a config without changelog sections', () => {
    expect(() => changelogTypes('{"packages":{".":{}}}')).toThrow();
  });
});

describe('prTitleProblem', () => {
  it('accepts the titles this repository merges, its release pull requests and Dependabot', () => {
    for (const title of [
      'fix(input): map the Back key on Tizen 8',
      'feat(devsupport): Fast Refresh, reload, logs and LogBox from expo start',
      'fix(input,tv,text): decode UTF-8 per WHATWG, finish held presses',
      'docs: a README for the first release',
      'feat(cli)!: rename --device to --tv',
      'chore: release 0.1.0-alpha.1',
      'chore(deps-dev): bump the dev-tools group with 3 updates',
    ]) {
      expect(prTitleProblem(title, types), title).toBeUndefined();
    }
  });

  it('refuses a title that is not type(scope): summary', () => {
    for (const title of [
      'Map the Back key on Tizen 8',
      'fix map the Back key',
      'fix(): map the Back key',
      'fix(Input): map the Back key',
      'fix(input, tv): map the Back key',
      'fix(input):map the Back key',
      'fix(input): ',
      'Revert "feat(cli): rn-tizen init"',
    ]) {
      expect(prTitleProblem(title, types), title).toBe(
        'The title is not in the form type(scope): summary',
      );
    }
  });

  it('refuses a type the changelog does not know', () => {
    expect(prTitleProblem('feature(cli): add init', types)).toBe(
      '"feature" is not a commit type the changelog knows',
    );
  });
});
