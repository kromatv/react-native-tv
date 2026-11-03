import { networkInterfaces } from 'node:os';
import { join } from 'node:path';
import { COMMON_OPTIONS, parseCommandArgs, type Command } from '../cli/command.ts';
import { LEVEL_OPTIONS, parseLevel, untilInterrupted } from '../cli/log-level-option.ts';
import { parsePort } from '../cli/log-options.ts';
import { Reporter } from '../cli/reporter.ts';
import { pickLanAddress } from '../logs/lan-address.ts';
import { crashSymbolizer } from '../logs/llvm-symbolizer.ts';
import { LogPrinter } from '../logs/log-printer.ts';
import { RUNTIME_LIBRARY } from '../runtime/build-runtime.ts';
import { listenForLogs } from '../logs/log-server.ts';

const OPTIONS = {
  port: {
    type: 'string',
    placeholder: '<port>',
    description: 'The port the app streams to, as packaged (default: 8099)',
  },
  ...LEVEL_OPTIONS,
  ...COMMON_OPTIONS,
} as const;

/** `rn-tizen logs`: prints what a development build streams, until Ctrl+C. */
export const logsCommand: Command = {
  name: 'logs',
  summary: 'Print the logs a development build streams, until Ctrl+C',
  options: OPTIONS,
  examples: ['rn-tizen logs', 'rn-tizen logs --level debug', 'rn-tizen logs --port 9000'],
  async run(args, context) {
    const values = parseCommandArgs('logs', args, OPTIONS);
    const reporter = new Reporter({ verbose: values.verbose ?? false });
    const minimum = parseLevel(values.level, 'logs');
    const printer = new LogPrinter({
      paint: reporter.paint,
      minimum,
      held: false,
      transform: crashSymbolizer(join(context.cwd, 'tizen', 'lib', RUNTIME_LIBRARY)),
    });
    const server = await listenForLogs(parsePort(values.port, '--port', 'logs'), printer);
    const address = pickLanAddress(networkInterfaces(), null) ?? 'this computer';
    reporter.line(`Listening on ${address}:${server.port}. Press Ctrl+C to stop.`);
    reporter.note(
      'Launch a development build on the TV (rn-tizen run, or from its apps) to see its logs.',
    );
    await untilInterrupted();
    await server.close();
  },
};
