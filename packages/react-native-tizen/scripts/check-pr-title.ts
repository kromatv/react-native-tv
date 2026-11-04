import { readFileSync } from 'node:fs';
import { changelogTypes, prTitleProblem } from './pr-title.ts';

const types = changelogTypes(readFileSync('release-please-config.json', 'utf8'));
const problem = prTitleProblem(process.argv[2] ?? '', types);
if (problem !== undefined) {
  console.log(
    `::error title=Pull request title::${problem}. The title becomes the squash commit and its changelog line, so it reads type(scope): summary, such as fix(input): map the Back key on Tizen 8. The type is one of ${types.join(', ')}; the scope is optional.`,
  );
  process.exit(1);
}
