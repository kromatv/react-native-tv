import { fetch } from 'expo/fetch';
import { attempt, check, type Check } from './check-list.tsx';

const ZEN = 'https://api.github.com/zen';
const JSON_URL = 'https://api.github.com/repos/facebook/react-native';

export function fetchChecks(): Promise<readonly Check[]> {
  return Promise.all([
    attempt('expo/fetch text()', async () => {
      const response = await fetch(ZEN);
      const text = await response.text();
      return check(
        'expo/fetch text()',
        response.ok && text.length > 0,
        `${response.status} ${text.trim()}`,
      );
    }),
    attempt('expo/fetch json() and headers', async () => {
      const response = await fetch(JSON_URL, { headers: { Accept: 'application/json' } });
      const body: unknown = await response.json();
      const name =
        typeof body === 'object' && body !== null ? Reflect.get(body, 'full_name') : undefined;
      return check(
        'expo/fetch json() and headers',
        typeof name === 'string' && name.endsWith('/react-native'),
        {
          name,
          type: response.headers.get('content-type'),
        },
      );
    }),
    attempt('expo/fetch body stream', async () => {
      const response = await fetch(ZEN);
      const reader = response.body?.getReader();
      let bytes = 0;
      while (reader) {
        const { done, value } = await reader.read();
        if (done) break;
        bytes += value.length;
      }
      return check('expo/fetch body stream', bytes > 0, `${bytes} bytes`);
    }),
    attempt('expo/fetch abort rejects', async () => {
      const controller = new AbortController();
      const pending = fetch(ZEN, { signal: controller.signal });
      controller.abort();
      const outcome = await pending.then(
        () => 'resolved',
        (error: unknown) => (error instanceof Error ? `rejected: ${error.message}` : 'rejected'),
      );
      return check('expo/fetch abort rejects', outcome.startsWith('rejected'), outcome);
    }),
  ]);
}
