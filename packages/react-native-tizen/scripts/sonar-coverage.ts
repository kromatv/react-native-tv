import { relative } from 'node:path';

const MAX_LCOV_BYTES = 256 * 1024 * 1024;
const SOURCE_FILE = /^SF:(.+)$/;
const LINE = /^DA:(\d+),(\d+)(?:,.*)?$/;
const BRANCH = /^BRDA:(\d+),([^,]+,[^,]+),(-|\d+)$/;

interface Line {
  hits: number;
  readonly branches: Map<string, boolean>;
}

/** A coverage report in SonarQube's generic format, and how many files it covers. */
export interface SonarCoverage {
  readonly xml: string;
  readonly files: number;
}

function escapeXml(text: string): string {
  return text.replaceAll('&', '&amp;').replaceAll('"', '&quot;').replaceAll('<', '&lt;');
}

function lineOf(lines: Map<number, Line>, number: number): Line {
  let line = lines.get(number);
  if (line === undefined) {
    line = { hits: 0, branches: new Map() };
    lines.set(number, line);
  }
  return line;
}

function fileXml(path: string, lines: Map<number, Line>): string {
  const entries = [...lines.entries()]
    .sort(([a], [b]) => a - b)
    .map(([number, line]) => {
      const taken = [...line.branches.values()].filter(Boolean).length;
      const branches =
        line.branches.size === 0
          ? ''
          : ` branchesToCover="${line.branches.size}" coveredBranches="${taken}"`;
      return `    <lineToCover lineNumber="${number}" covered="${line.hits > 0}"${branches}/>`;
    });
  return [`  <file path="${escapeXml(path)}">`, ...entries, '  </file>'].join('\n');
}

function readRecord(lines: Map<number, Line>, text: string): void {
  const line = LINE.exec(text);
  if (line !== null) {
    lineOf(lines, Number(line[1])).hits += Number(line[2]);
    return;
  }
  const branch = BRANCH.exec(text);
  if (branch === null) return;
  const { branches } = lineOf(lines, Number(branch[1]));
  const id = branch[2] ?? '';
  const taken = branch[3] !== '-' && Number(branch[3]) > 0;
  branches.set(id, (branches.get(id) ?? false) || taken);
}

/**
 * Converts `llvm-cov export -format=lcov` output into SonarQube's generic test
 * coverage XML, keeping the files under `root` that `keep` accepts, by their
 * path relative to `root`. A file listed by several objects is merged.
 */
export function lcovToSonar(
  lcov: string,
  root: string,
  keep: (path: string) => boolean,
): SonarCoverage {
  if (lcov.length > MAX_LCOV_BYTES)
    throw new Error(`The lcov report is over ${MAX_LCOV_BYTES} bytes`);
  const files = new Map<string, Map<number, Line>>();
  let current: Map<number, Line> | null = null;
  for (const text of lcov.split('\n')) {
    const source = SOURCE_FILE.exec(text);
    if (source === null) {
      if (current !== null) readRecord(current, text);
      continue;
    }
    const path = relative(root, source[1] ?? '');
    current = keep(path) ? (files.get(path) ?? new Map()) : null;
    if (current !== null) files.set(path, current);
  }
  const body = [...files.entries()]
    .sort(([a], [b]) => a.localeCompare(b))
    .map(([path, lines]) => fileXml(path, lines));
  return {
    xml: ['<coverage version="1">', ...body, '</coverage>', ''].join('\n'),
    files: files.size,
  };
}
