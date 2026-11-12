const base = `${location.pathname.replace(/\/(index\.html)?$/, '')}/`;
const elements = {
  jobs: document.getElementById('jobs'),
  title: document.getElementById('title'),
  status: document.getElementById('status'),
  stop: document.getElementById('stop'),
  log: document.getElementById('log'),
};
let selected = new URLSearchParams(location.search).get('job');
let offset = 0;

async function getJson(path, init) {
  const response = await fetch(`${base}${path}`, init);
  return response.json();
}

function renderJobs(jobs) {
  elements.jobs.replaceChildren(
    ...jobs.map((job) => {
      const link = document.createElement('a');
      link.href = `?job=${job.id}`;
      link.textContent = job.title;
      if (job.id === selected) link.setAttribute('aria-current', 'page');
      const status = document.createElement('div');
      status.className = 'status';
      status.textContent = `${job.status}, ${new Date(job.startedAt).toLocaleTimeString()}`;
      link.append(status);
      link.addEventListener('click', (event) => {
        event.preventDefault();
        select(job.id);
      });
      return link;
    }),
  );
}

function select(id) {
  selected = id;
  offset = 0;
  elements.log.textContent = '';
  history.replaceState(null, '', `?job=${id}`);
}

async function refresh() {
  const { jobs } = await getJson('api/jobs');
  selected ??= jobs[0]?.id ?? null;
  renderJobs(jobs);
  if (selected === null) return;
  const { job, text, next } = await getJson(`api/jobs/${selected}/log?from=${offset}`);
  if (job === undefined) return;
  const atBottom =
    elements.log.scrollTop + elements.log.clientHeight >= elements.log.scrollHeight - 8;
  elements.log.textContent += text;
  offset = next;
  if (atBottom) elements.log.scrollTop = elements.log.scrollHeight;
  elements.title.textContent = job.title;
  elements.status.textContent = job.status;
  elements.stop.hidden = job.status !== 'running';
}

elements.stop.addEventListener('click', async () => {
  if (selected !== null) await getJson(`api/jobs/${selected}/stop`, { method: 'POST' });
});

async function loop() {
  try {
    await refresh();
  } catch (error) {
    elements.status.textContent = `The dev server did not answer: ${error}`;
  }
  setTimeout(loop, 1000);
}

loop();
