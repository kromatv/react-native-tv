import { jobsDir, listJobs, readJob, readLog, stopJob } from './jobs.ts';

const JOB_ROUTE = /^\/api\/jobs\/([a-z0-9-]{1,64})\/(log|stop)$/;

function json(body: unknown, status = 200): Response {
  return new Response(JSON.stringify(body), {
    status,
    headers: { 'content-type': 'application/json', 'cache-control': 'no-store' },
  });
}

async function jobRoute(request: Request, id: string, action: string): Promise<Response> {
  const dir = jobsDir();
  if (action === 'stop') {
    if (request.method !== 'POST') return json({ error: 'POST only' }, 405);
    return json({ job: await stopJob(dir, id) });
  }
  const job = await readJob(dir, id);
  if (job === null) return json({ error: 'no such job' }, 404);
  const from = Number(new URL(request.url).searchParams.get('from') ?? '0');
  const log = await readLog(dir, id, Number.isSafeInteger(from) && from >= 0 ? from : 0);
  return json({ job, ...log });
}

/**
 * The dev tools plugin's server inside `expo start`: the jobs its commands
 * started, their logs, and a way to stop one. Any other path falls through to
 * the page in `devtools/`.
 */
export default async function handler(request: Request): Promise<Response | undefined> {
  const { pathname } = new URL(request.url);
  if (pathname === '/api/jobs') return json({ jobs: await listJobs(jobsDir()) });
  const match = JOB_ROUTE.exec(pathname);
  if (match?.[1] === undefined || match[2] === undefined) return undefined;
  return jobRoute(request, match[1], match[2]);
}
