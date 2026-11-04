import { singleReactNativeIssue } from '../dependencies/react-native-copies.ts';
import { tizenMetroIssue } from '../metro/metro-platforms.ts';
import type { JobPlan } from './devtools-commands.ts';
import { listJobs, startJob, stopJob, type Job } from './jobs.ts';

/**
 * Starts a planned job in the app at `root`, after checking that a job that
 * bundles has an app Metro can bundle for Tizen, and stopping the running job
 * of an exclusive kind.
 */
export async function startPlannedJob(dir: string, plan: JobPlan, root: string): Promise<Job> {
  const setupIssue = plan.bundles ? (singleReactNativeIssue(root) ?? tizenMetroIssue(root)) : null;
  if (setupIssue !== null) throw setupIssue;
  if (plan.exclusive) {
    const running = (await listJobs(dir)).filter(
      (job) => job.kind === plan.kind && job.status === 'running',
    );
    await Promise.all(running.map((job) => stopJob(dir, job.id)));
  }
  return startJob(dir, { kind: plan.kind, title: plan.title, args: [...plan.args], cwd: root });
}
