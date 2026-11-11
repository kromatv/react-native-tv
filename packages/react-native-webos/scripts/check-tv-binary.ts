import { readFileSync } from 'node:fs';
import { parseArgs } from 'node:util';
import { FixableError } from '../src/errors.ts';
import { parseElf } from './tv-binary/elf-file.ts';
import { ensureFirmwareData, loadFirmwares } from './tv-binary/firmware-data.ts';
import { formatTvBinaryReport, reportTvBinary } from './tv-binary/tv-binary-report.ts';

// docs/decisions.md and docs/reference/webos-native-toolchain.md, "Floor".
const CLAIMED_FLOOR = 4;

const USAGE = `Usage: bun scripts/check-tv-binary.ts <executable> [--opens <soname>]... [--floor <major>] [--json]

Reads the react-native-webos executable a TV build produced and reports what a
TV's dynamic linker will ask of it: the ABI, the interpreter, the libraries and
symbol versions it needs, and on which webOS releases all of that exists,
against webosbrew's firmware symbol data.

  --opens <soname>  a library the code opens with dlopen and cannot run without
  --floor <major>   the oldest webOS release that must pass (default ${CLAIMED_FLOOR})
  --json            print the report as JSON`;

async function main(): Promise<number> {
  const { values, positionals } = parseArgs({
    allowPositionals: true,
    options: {
      opens: { type: 'string', multiple: true, default: [] },
      floor: { type: 'string', default: String(CLAIMED_FLOOR) },
      json: { type: 'boolean', default: false },
      help: { type: 'boolean', short: 'h', default: false },
    },
  });
  const [path] = positionals;
  if (values.help || path === undefined || positionals.length !== 1) {
    console.log(USAGE);
    return values.help ? 0 : 2;
  }
  const claimed = Number(values.floor);
  if (!Number.isInteger(claimed) || claimed < 1) {
    throw new FixableError(`--floor takes a webOS major release, got '${values.floor}'`, USAGE);
  }
  const elf = parseElf(readFileSync(path));
  const firmwares = loadFirmwares(await ensureFirmwareData());
  const report = reportTvBinary(elf, firmwares, values.opens);
  if (values.json) {
    const lookups = (map: ReadonlyMap<string, readonly string[]>) => Object.fromEntries(map);
    const runtimeLibraries = report.runtimeLibraries.map((library) => ({
      ...library,
      lookups: lookups(library.lookups),
    }));
    console.log(JSON.stringify({ ...report, runtimeLibraries }, null, 2));
  } else {
    console.log(formatTvBinaryReport(report));
  }
  const measured = report.floor === null ? null : Number(report.floor.split('.')[0]);
  if (measured !== null && measured <= claimed) {
    if (!values.json) console.log(`The claimed floor, webOS ${claimed}, holds.`);
    return 0;
  }
  console.error(
    `The claimed floor, webOS ${claimed}, does not hold: ${
      measured === null
        ? 'the newest image lacks something'
        : `the first release that passes is ${report.floor}`
    }.`,
  );
  return 1;
}

try {
  process.exitCode = await main();
} catch (error) {
  if (!(error instanceof FixableError)) throw error;
  console.error(`${error.message}\n${error.hint}`);
  process.exitCode = 3;
}
