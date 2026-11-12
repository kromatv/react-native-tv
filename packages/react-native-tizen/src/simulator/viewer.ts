import { FixableError } from '../errors.ts';
import type { Task } from '../task.ts';
import type { SimulatorInstance } from './instance.ts';
import { runInstanceScript } from './simulator.ts';

const FORWARD_TIMEOUT_MS = 15_000;
const FORWARD_POLL_MS = 250;

async function waitForForward(url: string): Promise<void> {
  const deadline = Date.now() + FORWARD_TIMEOUT_MS;
  while (Date.now() < deadline) {
    const reachable = await fetch(url).then(
      (response) => response.ok,
      () => false,
    );
    if (reachable) return;
    await new Promise((resolve) => setTimeout(resolve, FORWARD_POLL_MS));
  }
  throw new FixableError(
    `The simulator's live screen did not come up at ${url}`,
    'Check the instance with `rn-tizen simulator status`, then run `rn-tizen simulator view` again.',
  );
}

/** Where the live screen of `instance` is on this Mac. */
export function viewerUrl(instance: SimulatorInstance): string {
  return `http://127.0.0.1:${instance.viewerPort}/`;
}

/**
 * Serves the live screen and the remote of `instance` from the VM and returns
 * their URL on this Mac, once Lima forwards it. Repeating it reuses a running viewer.
 */
export async function startViewer(task: Task, instance: SimulatorInstance): Promise<string> {
  await runInstanceScript(task, instance, 'view');
  const url = viewerUrl(instance);
  await waitForForward(url);
  return url;
}

/** Opens `url` in the Mac's default browser. */
export async function openInBrowser(task: Task, url: string): Promise<void> {
  await task.run(['/usr/bin/open', url]);
}
