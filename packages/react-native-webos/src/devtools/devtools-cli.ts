import { runDevtoolsCommand } from './run-devtools-command.ts';

const { lines, exitCode } = await runDevtoolsCommand(process.argv.slice(2));
process.stdout.write(`${JSON.stringify(lines)}\n`);
process.exitCode = exitCode;
