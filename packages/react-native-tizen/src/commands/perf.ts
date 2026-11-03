import { COMMON_OPTIONS, parseCommandArgs, type Command } from '../cli/command.ts';
import { deviceAddress } from '../cli/device-option.ts';
import { parsePort } from '../cli/log-options.ts';
import { Reporter } from '../cli/reporter.ts';
import { parsePerfFlags, PERF_OPTIONS } from '../perf/perf-options.ts';
import { watchPerf } from '../perf/watch-perf.ts';

const OPTIONS = {
  device: {
    type: 'string',
    short: 'd',
    placeholder: '<ip>',
    description: 'Only read the app on this TV (default: any that connects)',
  },
  port: {
    type: 'string',
    placeholder: '<port>',
    description: 'The port the app streams its logs to, as packaged (default: 8099)',
  },
  ...PERF_OPTIONS,
  out: {
    type: 'string',
    placeholder: '<file.jsonl>',
    description: 'Also save every sample, one JSON object per line',
  },
  level: {
    type: 'string',
    placeholder: '<level>',
    description: 'The quietest log level to print between samples (default: warn)',
  },
  ...COMMON_OPTIONS,
} as const;

const IN_USE_HINT =
  'rn-tizen perf listens on the log port itself: stop the `rn-tizen run` or `logs` holding it (run --no-logs launches without one).';

/** `rn-tizen perf`: prints a development build's frame, thread and heap timings once a second. */
export const perfCommand: Command = {
  name: 'perf',
  summary: "Print a development build's frame times, thread load and heap every second",
  options: OPTIONS,
  examples: [
    'rn-tizen perf --device 192.168.1.50',
    'rn-tizen perf --port 8102 --duration 20 --out boot.jsonl',
  ],
  async run(args, context) {
    const flags = parseCommandArgs('perf', args, OPTIONS);
    const reporter = new Reporter({ verbose: flags.verbose ?? false });
    await watchPerf(reporter, {
      ...parsePerfFlags(flags, 'perf', context.cwd),
      port: parsePort(flags.port, '--port', 'perf'),
      device: flags.device === undefined ? null : deviceAddress(flags.device, 'perf'),
      inUseHint: IN_USE_HINT,
    });
  },
};
