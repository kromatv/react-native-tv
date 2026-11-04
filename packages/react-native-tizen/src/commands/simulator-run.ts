import { existsSync } from 'node:fs';
import { relative, resolve } from 'node:path';
import { loadAppProject, type AppProject } from '../app/app-project.ts';
import { entryPath, type CommandContext } from '../cli/command.ts';
import { parseLevel, untilInterrupted } from '../cli/log-level-option.ts';
import { parsePort } from '../cli/log-options.ts';
import { FixableError } from '../errors.ts';
import { Reporter } from '../cli/reporter.ts';
import { LogPrinter } from '../logs/log-printer.ts';
import { listenForLogs, type LogServer } from '../logs/log-server.ts';
import { bundleDir, bundleStep, appSetupSteps } from '../pipeline/package-steps.ts';
import { devServerBundleStep } from '../package/dev-server-bundle.ts';
import { simulatorCommandLine, type SimulatorInstance } from '../simulator/instance.ts';
import { ensureVm, hostAddress, startApp, type VmPreparation } from '../simulator/simulator.ts';
import { ensureIcuData } from '../runtime/icu-data.ts';
import { buildSimulatorRuntime } from '../simulator/simulator-runtime.ts';
import { stageApp, type SimulatorBundle } from '../simulator/stage-app.ts';
import { openInBrowser, startViewer, viewerUrl } from '../simulator/viewer.ts';

/** The flags `rn-tizen simulator run` and `build` read. */
export interface SimulatorRunFlags {
  readonly project?: string;
  readonly entry?: string;
  readonly bundle?: string;
  readonly gui?: boolean;
  readonly 'no-logs'?: boolean;
  readonly 'log-port'?: string;
  readonly 'dev-server'?: string;
  readonly level?: string;
}

const PREPARATION_SUMMARY: Readonly<Record<VmPreparation, string>> = {
  created: 'created and provisioned',
  started: 'started',
  running: 'running',
};

/** An app, and the key of the simulator runtime built for its native libraries. */
export interface PreparedApp {
  readonly app: AppProject;
  readonly runtime: string;
}

/** Opens the app and makes the VM and the simulator runtime ready for it. */
export async function prepareSimulator(
  reporter: Reporter,
  flags: SimulatorRunFlags,
  context: CommandContext,
): Promise<PreparedApp> {
  const app = loadAppProject(resolve(context.cwd, flags.project ?? '.'), entryPath(flags, context));
  const where = relative(context.cwd, app.root);
  const location = where === '' ? '' : `  ${reporter.paint('gray', where)}`;
  reporter.field('App', `${app.config.packageId} ${app.config.version}${location}`);
  if (flags.bundle === undefined) await appSetupSteps(reporter, app);
  await reporter.step(
    'Simulator VM',
    (task) => ensureVm(task, app),
    (preparation) => PREPARATION_SUMMARY[preparation],
  );
  const runtime = await reporter.step(
    'Simulator runtime',
    (task) =>
      buildSimulatorRuntime(task, app, () =>
        reporter.note('Another simulator build is running: waiting for it to finish.'),
      ),
    (key) => (key === '' ? 'aarch64 Linux' : `aarch64 Linux, ${key}`),
  );
  return { app, runtime };
}

function givenBytecode(path: string): SimulatorBundle {
  if (existsSync(path)) return { bytecode: path };
  throw new FixableError(
    `The bytecode ${path} does not exist`,
    'Pass --bundle the .hbc that `rn-tizen bundle` wrote, or leave it out to bundle the app.',
  );
}

function describeInstance(instance: SimulatorInstance): string {
  return `${instance.name}  screen :${instance.display}, live screen ${viewerUrl(instance)}`;
}

function listenForInstanceLogs(
  instance: SimulatorInstance,
  flags: SimulatorRunFlags,
  printer: LogPrinter,
): Promise<LogServer> {
  const port = parsePort(flags['log-port'], '--log-port', 'simulator', instance.logPort);
  const busy = `Another \`${simulatorCommandLine('run', instance.name)}\` is probably printing its logs: stop it, pass --no-logs, or pass --log-port <port>.`;
  return listenForLogs(port, printer, { inUseHint: busy });
}

async function simulatorBundle(
  reporter: Reporter,
  flags: SimulatorRunFlags,
  app: AppProject,
  context: CommandContext,
): Promise<SimulatorBundle> {
  const devServer = flags['dev-server'];
  if (devServer !== undefined)
    return devServerBundleStep(reporter, app, bundleDir(app), devServer, hostAddress);
  if (flags.bundle !== undefined) return givenBytecode(resolve(context.cwd, flags.bundle));
  return bundleStep(reporter, app);
}

/**
 * `rn-tizen simulator run`: builds what changed, bundles the app, starts it on
 * the virtual TV of `instance` and prints its logs until Ctrl+C. The app keeps
 * running after that, for `key`, `screenshot` and `view`; no other instance
 * is touched.
 */
export async function runInSimulator(
  reporter: Reporter,
  flags: SimulatorRunFlags,
  context: CommandContext,
  instance: SimulatorInstance,
): Promise<void> {
  const minimum = parseLevel(flags.level, 'simulator');
  const printer = new LogPrinter({ paint: reporter.paint, minimum, held: true });
  const server: LogServer | null =
    flags['no-logs'] === true ? null : await listenForInstanceLogs(instance, flags, printer);
  try {
    reporter.field('Instance', describeInstance(instance));
    const { app, runtime } = await prepareSimulator(reporter, flags, context);
    const bundle: SimulatorBundle = await simulatorBundle(reporter, flags, app, context);
    await reporter.step(
      'Start',
      async (task) => {
        const log = server === null ? null : { host: await hostAddress(task), port: server.port };
        const icuData = await ensureIcuData(task, app.config.locales);
        await stageApp(instance, app, bundle, log, { key: runtime, icuData });
        return startApp(task, instance);
      },
      (pid) => `pid ${pid}`,
    );
    if (flags.gui === true) {
      await reporter.step(
        'Live screen',
        async (task) => {
          const url = await startViewer(task, instance);
          await openInBrowser(task, url);
          return url;
        },
        (url) => url,
      );
    }
    if (server === null) return;
    reporter.line();
    reporter.note('Logs from the simulator. Ctrl+C stops printing them; the app keeps running.');
    printer.release();
    await untilInterrupted();
  } finally {
    await server?.close();
  }
}
