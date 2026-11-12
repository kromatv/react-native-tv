import { join } from 'node:path';
import { z } from 'zod';
import { cacheDir } from '../cache-dir.ts';
import {
  COMMON_OPTIONS,
  ENTRY_OPTIONS,
  parseCommandArgs,
  PROJECT_OPTIONS,
  UsageError,
  type Command,
  type CommandContext,
  type OptionSpecs,
} from '../cli/command.ts';
import { LEVEL_OPTIONS } from '../cli/log-level-option.ts';
import { parsePort } from '../cli/log-options.ts';
import { Reporter } from '../cli/reporter.ts';
import { closestMatch } from '../cli/suggest.ts';
import { parsePerfFlags, PERF_OPTIONS } from '../perf/perf-options.ts';
import { watchPerf } from '../perf/watch-perf.ts';
import type { Task } from '../task.ts';
import { splitActionArgs } from '../simulator/action-args.ts';
import {
  chooseInstance,
  DEFAULT_INSTANCE,
  INSTANCE_ENV,
  simulatorCommandLine,
  type SimulatorInstance,
} from '../simulator/instance.ts';
import { launchParams } from '../simulator/launch-params.ts';
import { SCREEN_SIZES, screenSize } from '../simulator/screen-size.ts';
import { deleteInstance, requireLima } from '../simulator/lima.ts';
import {
  crashReport,
  openUrl,
  pressKeys,
  printLog,
  requireRunningVm,
  saveScreenshot,
  sendControl,
  sendLifecycle,
  sendPointer,
  type PointerCommand,
  startApp,
  stopSimulator,
} from '../simulator/simulator.ts';
import { cleanSimulatorRuntimes } from '../simulator/simulator-runtime.ts';
import { openInBrowser, startViewer } from '../simulator/viewer.ts';
import { prepareSimulator, runInSimulator } from './simulator-run.ts';
import { printSimulatorStatus } from './simulator-status.ts';

const DEFAULT_LOG_LINES = 200;
const MAX_LOG_LINES = 100_000;

const lineCountSchema = z.coerce.number().int().nonnegative().max(MAX_LOG_LINES);
const coordinateSchema = z.coerce.number().int().min(0).max(8192);
const pointSchema = z.tuple([coordinateSchema, coordinateSchema]);
const wheelSchema = z.coerce.number().min(-100).max(100);

const OPTIONS = {
  instance: {
    type: 'string',
    placeholder: '<name>',
    description: `The instance to act on, each with its own screen, app and ports (default: $${INSTANCE_ENV}, else ${DEFAULT_INSTANCE})`,
  },
  bundle: {
    type: 'string',
    placeholder: '<file.hbc>',
    description: 'run: start this Hermes bytecode instead of bundling the app',
  },
  'dev-server': {
    type: 'string',
    placeholder: '<origin>',
    description:
      'run: load the bundle from this `expo start` dev server instead of bundling the app: Fast Refresh, r to reload, LogBox and logs in its terminal',
  },
  gui: { type: 'boolean', description: 'run: also open the live screen and remote in the browser' },
  params: {
    type: 'string',
    placeholder: '<json>',
    description:
      'run, restart, relaunch: the parameters the app is launched with, as a TV passes them: \'{"contentTarget":"myapp://item/42"}\'',
  },
  screen: {
    type: 'string',
    placeholder: '<size>',
    description: `run, restart: the screen the app gets, ${SCREEN_SIZES[0]} as on a UHD TV or ${SCREEN_SIZES[1]} as on a Full HD TV (default: the instance's last, else ${SCREEN_SIZES[0]})`,
  },
  'no-logs': {
    type: 'boolean',
    description: 'run: exit once the app is up instead of printing its logs',
  },
  'log-port': {
    type: 'string',
    placeholder: '<port>',
    description:
      "run, perf: the port on this Mac the app streams its logs to (default: the instance's own, see status)",
  },
  ...LEVEL_OPTIONS,
  ...PERF_OPTIONS,
  out: {
    type: 'string',
    placeholder: '<file>',
    description:
      'screenshot: where to save the PNG (default: simulator/<instance>/screenshot.png in the cache); perf: where to save the samples',
  },
  follow: { type: 'boolean', short: 'f', description: 'logs: keep printing until Ctrl+C' },
  lines: {
    type: 'string',
    short: 'n',
    placeholder: '<count>',
    description: `logs: how many lines to print first (default: ${DEFAULT_LOG_LINES})`,
  },
  ...PROJECT_OPTIONS,
  ...ENTRY_OPTIONS,
  ...COMMON_OPTIONS,
} as const satisfies OptionSpecs;

type Flags = ReturnType<typeof parseCommandArgs<typeof OPTIONS>>;

interface Invocation {
  readonly positionals: readonly string[];
  readonly flags: Flags;
  readonly instance: SimulatorInstance;
  readonly reporter: Reporter;
  readonly context: CommandContext;
}

type Action = (invocation: Invocation) => Promise<void>;

function lineCount(value: string | undefined): number {
  if (value === undefined) return DEFAULT_LOG_LINES;
  const parsed = lineCountSchema.safeParse(value);
  if (parsed.success) return parsed.data;
  throw new UsageError(`--lines must be a whole number of lines, got ${value}`, 'simulator');
}

function pointerMove(words: readonly string[]): { type: 'move'; x: number; y: number } {
  const parsed = pointSchema.safeParse(words);
  if (parsed.success) return { type: 'move', x: parsed.data[0], y: parsed.data[1] };
  throw new UsageError(
    `The pointer takes x and y in screen pixels, got ${words.join(' ')}`,
    'simulator',
  );
}

function requirePositionals(invocation: Invocation, what: string): readonly string[] {
  if (invocation.positionals.length > 0) return invocation.positionals;
  throw new UsageError(`simulator needs ${what}`, 'simulator');
}

async function inRunningVm(
  reporter: Reporter,
  title: string,
  work: (task: Task) => Promise<unknown>,
  summary = '',
): Promise<void> {
  await reporter.step(
    title,
    async (task) => {
      await requireRunningVm(task);
      await work(task);
    },
    () => summary,
  );
}

const ACTIONS: Readonly<Record<string, Action>> = {
  run: ({ flags, reporter, context, instance }) =>
    runInSimulator(reporter, flags, context, instance),
  build: async ({ flags, reporter, context }) => {
    await prepareSimulator(reporter, flags, context);
  },
  view: async ({ reporter, instance }) => {
    await reporter.step(
      'Live screen',
      async (task) => {
        await requireRunningVm(task);
        const url = await startViewer(task, instance);
        await openInBrowser(task, url);
        return url;
      },
      (url) => url,
    );
    reporter.note('Arrows, Enter and Esc work as the remote while the page has the focus.');
  },
  screenshot: async ({ flags, reporter, instance }) => {
    const path = flags.out ?? join(cacheDir(), 'simulator', instance.name, 'screenshot.png');
    await reporter.step(
      'Screenshot',
      async (task) => {
        await requireRunningVm(task);
        return saveScreenshot(instance, path);
      },
      (saved) => saved,
    );
  },
  key: (invocation) => {
    const keys = requirePositionals(invocation, 'the keys to press, e.g. `key Down Enter`');
    return inRunningVm(
      invocation.reporter,
      'Keys',
      (task) => pressKeys(task, invocation.instance, keys),
      keys.join(' '),
    );
  },
  pause: ({ reporter, instance }) =>
    inRunningVm(reporter, 'Pause', (task) => sendLifecycle(task, instance, 'pause')),
  resume: ({ reporter, instance }) =>
    inRunningVm(reporter, 'Resume', (task) => sendLifecycle(task, instance, 'resume')),
  'open-url': (invocation) => {
    const [url = ''] = requirePositionals(invocation, 'a URL, e.g. `open-url myapp://item/42`');
    return inRunningVm(
      invocation.reporter,
      'Deep link',
      (task) => openUrl(task, invocation.instance, url),
      url,
    );
  },
  relaunch: ({ flags, reporter, instance }) => {
    const params = launchParams(flags.params) ?? '{}';
    return inRunningVm(
      reporter,
      'Relaunch',
      (task) => sendControl(task, instance, 'relaunch', [params]),
      params,
    );
  },
  text: (invocation) => {
    const text = requirePositionals(
      invocation,
      'the text the on-screen keyboard types, e.g. `text "Blade Runner"`',
    ).join(' ');
    return inRunningVm(
      invocation.reporter,
      'Text',
      (task) => sendControl(task, invocation.instance, 'text', [text]),
      text,
    );
  },
  pointer: (invocation) => {
    const words = requirePositionals(
      invocation,
      'where the pointer goes, in screen pixels, e.g. `pointer 960 540`',
    );
    const move = pointerMove(words);
    return inRunningVm(
      invocation.reporter,
      'Pointer',
      (task) => sendPointer(task, invocation.instance, [move]),
      `${move.x}, ${move.y}`,
    );
  },
  click: (invocation) => {
    const move = invocation.positionals.length > 0 ? [pointerMove(invocation.positionals)] : [];
    const commands: PointerCommand[] = [...move, { type: 'down' }, { type: 'up' }];
    return inRunningVm(invocation.reporter, 'Click', (task) =>
      sendPointer(task, invocation.instance, commands),
    );
  },
  wheel: (invocation) => {
    const [notches = ''] = requirePositionals(
      invocation,
      'how far the wheel turns, e.g. `wheel -3` to scroll down three notches',
    );
    const parsed = wheelSchema.safeParse(notches);
    if (!parsed.success) {
      throw new UsageError(`wheel takes a number of notches, got ${notches}`, 'simulator');
    }
    return inRunningVm(
      invocation.reporter,
      'Wheel',
      (task) => sendPointer(task, invocation.instance, [{ type: 'wheel', notches: parsed.data }]),
      notches,
    );
  },
  perf: ({ flags, reporter, instance, context }) =>
    watchPerf(reporter, {
      ...parsePerfFlags(flags, 'simulator', context.cwd),
      port: parsePort(flags['log-port'], '--log-port', 'simulator', instance.logPort),
      device: null,
      app: null,
      inUseHint: `Another \`${simulatorCommandLine('run', instance.name)}\` is printing its logs: stop it, or start the app with --no-logs.`,
    }).then(() => undefined),
  restart: async ({ flags, reporter, instance }) => {
    await reporter.step(
      'Restart',
      async (task) => {
        await requireRunningVm(task);
        return startApp(task, instance, {
          launchParams: launchParams(flags.params),
          screen: screenSize(flags.screen),
        });
      },
      (pid) => `pid ${pid}`,
    );
  },
  logs: async ({ flags, reporter, instance }) => {
    const lines = lineCount(flags.lines);
    await requireRunningVm(reporter.task());
    await printLog(instance, { lines, follow: flags.follow === true });
  },
  crash: async ({ reporter, instance }) => {
    const task = reporter.task();
    await requireRunningVm(task);
    reporter.line((await crashReport(task, instance)).trimEnd());
  },
  status: ({ reporter, instance }) => printSimulatorStatus(reporter, instance),
  stop: ({ reporter, instance }) =>
    inRunningVm(reporter, 'Stop', (task) => stopSimulator(task, instance), instance.name),
  clean: async ({ reporter }) => {
    await reporter.step(
      'Clean',
      async (task) => {
        await requireRunningVm(task);
        return cleanSimulatorRuntimes(task);
      },
      (removed) =>
        removed.length === 0 ? 'every runtime build is in use' : `removed ${removed.join(', ')}`,
    );
  },
  delete: async ({ reporter }) => {
    await reporter.step(
      'Delete VM',
      (task) => deleteInstance(task),
      () => 'rn-webos-sim, with every instance',
    );
  },
};

function findAction(name: string | undefined): Action {
  if (name === undefined) {
    throw new UsageError('simulator needs an action, e.g. `rn-webos simulator run`', 'simulator');
  }
  const action = ACTIONS[name];
  if (action !== undefined) return action;
  const suggestion = closestMatch(name, Object.keys(ACTIONS));
  const hint = suggestion === null ? '' : `. Did you mean ${suggestion}?`;
  throw new UsageError(`Unknown simulator action ${name}${hint}`, 'simulator');
}

/** `rn-webos simulator`: runs and drives the app on a virtual TV in a local VM. */
export const simulatorCommand: Command = {
  name: 'simulator',
  summary: 'Run the app on a virtual TV on this Mac, headless or with a live screen, and drive it',
  options: OPTIONS,
  examples: [
    'rn-webos simulator run                  build, bundle and start the app, then print its logs',
    'rn-webos simulator run --gui            the same, and open the live screen and remote',
    'rn-webos simulator run --dev-server http://localhost:8081  load the bundle from `expo start`, with Fast Refresh',
    'rn-webos simulator view                 open the live screen of the running app',
    'rn-webos simulator screenshot --out shot.png',
    'rn-webos simulator key Down Down Enter  Up Down Left Right Enter Back Play Pause Stop Red ...',
    'rn-webos simulator key Enter:800        hold a key for 800 ms',
    'rn-webos simulator text "Blade Runner"  type into the focused text field, as the on-screen keyboard does',
    "rn-webos simulator pointer 960 540      move the remote's pointer; click presses its button, wheel -3 scrolls",
    'rn-webos simulator pause                send the app to the background; resume brings it back',
    'rn-webos simulator open-url myapp://item/42',
    `rn-webos simulator relaunch --params '{"contentTarget":"myapp://item/42"}'  launch the running app again, as the TV does`,
    'rn-webos simulator restart              start the app again, without rebuilding',
    'rn-webos simulator build                rebuild the native runtime only',
    'rn-webos simulator clean                remove the runtime builds no instance runs',
    'rn-webos simulator logs -f',
    'rn-webos simulator crash                backtraces of every thread at the last native crash',
    'rn-webos simulator perf --duration 20   frame times, thread load and heap, every second',
    'rn-webos simulator status               the VM, the runtime and every instance',
    'rn-webos simulator stop                 stop the app and its screen; the VM keeps running',
    'rn-webos simulator delete               remove the VM, its builds and every instance',
    'rn-webos simulator run --instance ui-a  a second app on its own screen, beside the default one',
  ],
  async run(args, context) {
    const [name, ...rest] = args;
    const action = findAction(name);
    const { positionals, options } = splitActionArgs(rest, OPTIONS);
    const flags = parseCommandArgs('simulator', options, OPTIONS);
    const instance = chooseInstance(flags.instance, process.env);
    requireLima();
    const reporter = new Reporter({ verbose: flags.verbose ?? false });
    await action({ positionals, flags, instance, reporter, context });
  },
};
