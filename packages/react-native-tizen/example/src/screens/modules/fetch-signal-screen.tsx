import { fetch as expoFetch } from 'expo/fetch';
import { attempt, check, CheckList, useChecks, type Check } from '../expo/check-list.tsx';
import { ECHO_HTTP } from './echo-endpoint.ts';

const TIMEOUT_MS = 2500;

async function probe(base: string): Promise<string> {
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), TIMEOUT_MS);
  const started = Date.now();
  try {
    const response = await expoFetch(`${base}/utf8`, { signal: controller.signal });
    const text = await response.text();
    return `${response.status} ${text.length} chars in ${Date.now() - started} ms, url ${response.url}`;
  } catch (error) {
    const message = error instanceof Error ? `${error.name}: ${error.message}` : String(error);
    return `failed after ${Date.now() - started} ms: ${message}`;
  } finally {
    clearTimeout(timer);
  }
}

async function fetchSignalChecks(): Promise<readonly Check[]> {
  const insecure = ECHO_HTTP;
  const secure = ECHO_HTTP.replace(/^http:/, 'https:');
  return [
    await attempt('expo/fetch with an AbortSignal', async () => {
      const outcome = await probe(insecure);
      return check('expo/fetch with an AbortSignal', outcome.startsWith('200 '), outcome);
    }),
    await attempt('https and http probed at once', async () => {
      const [overTls, plain] = await Promise.all([probe(secure), probe(insecure)]);
      return check('https and http probed at once', plain.startsWith('200 '), { overTls, plain });
    }),
    await attempt('an abort rejects at once', async () => {
      const controller = new AbortController();
      const pending = expoFetch(`${insecure}/slow`, { signal: controller.signal });
      const started = Date.now();
      setTimeout(() => controller.abort(), 300);
      const outcome = await pending.then(
        () => 'resolved',
        (error: unknown) => (error instanceof Error ? error.name : 'rejected'),
      );
      const elapsed = Date.now() - started;
      return check(
        'an abort rejects at once',
        outcome !== 'resolved' && elapsed < 1500,
        `${outcome} after ${elapsed} ms`,
      );
    }),
  ];
}

export function FetchSignalScreen() {
  return <CheckList checks={useChecks('fetch-signal', fetchSignalChecks, 'modules')} />;
}
