import { fetch as expoFetch } from 'expo/fetch';
import { attempt, check, CheckList, useChecks, type Check } from '../expo/check-list.tsx';
import { ECHO_HTTP, UTF8_TEXT } from './echo-endpoint.ts';
import { readBlob } from './read-blob.ts';
import { within } from './socket-session.ts';

declare function require(id: 'react-native/Libraries/Network/fetch'): {
  readonly fetch: typeof fetch;
};
declare function require(id: 'react-native/Libraries/Blob/URL'): { readonly URL: typeof URL };

const reactNativeFetch = require('react-native/Libraries/Network/fetch').fetch;
const ReactNativeURL = require('react-native/Libraries/Blob/URL').URL;

function request(
  method: string,
  url: string,
  responseType: XMLHttpRequestResponseType,
  body?: Blob,
): Promise<XMLHttpRequest> {
  return within(
    `${method} ${url}`,
    new Promise((resolve, reject) => {
      const xhr = new XMLHttpRequest();
      xhr.open(method, url);
      xhr.responseType = responseType;
      xhr.onload = () => resolve(xhr);
      xhr.onerror = () => reject(new Error(`XHR failed: ${xhr.status}`));
      xhr.send(body ?? null);
    }),
  );
}

async function objectUrlCheck(label: string, create: (blob: Blob) => string): Promise<Check> {
  const blob = new Blob(['through a blob: URL']);
  const url = create(blob);
  const xhr = await request('GET', url, 'text');
  return check(label, url.startsWith('blob:') && xhr.responseText === 'through a blob: URL', url);
}

async function blobChecks(): Promise<readonly Check[]> {
  return [
    await attempt('new Blob(strings), readAsText', async () => {
      const blob = new Blob(['héllo', ' wörld ✓']);
      const text = await readBlob(blob, 'text');
      return check(
        'new Blob(strings), readAsText',
        blob.size === 17 && text === UTF8_TEXT,
        `${blob.size} bytes: ${text}`,
      );
    }),
    await attempt('readAsDataURL keeps the type', async () => {
      const url = await readBlob(new Blob(['hi'], { type: 'text/plain' }), 'dataURL');
      return check('readAsDataURL keeps the type', url === 'data:text/plain;base64,aGk=', url);
    }),
    await attempt('Blob.slice', async () => {
      const text = await readBlob(new Blob(['abcdefgh']).slice(1, 5), 'text');
      return check('Blob.slice', text === 'bcde', text);
    }),
    await attempt("React Native's URL.createObjectURL, then XHR", () =>
      objectUrlCheck("React Native's URL.createObjectURL, then XHR", (blob) =>
        ReactNativeURL.createObjectURL(blob),
      ),
    ),
    await attempt('global URL.createObjectURL (Expo), then XHR', () =>
      objectUrlCheck('global URL.createObjectURL (Expo), then XHR', (blob) =>
        URL.createObjectURL(blob),
      ),
    ),
    await attempt('XHR responseType blob', async () => {
      const xhr = await request('GET', `${ECHO_HTTP}/utf8`, 'blob');
      const blob: unknown = xhr.response;
      if (!(blob instanceof Blob)) return check('XHR responseType blob', false, typeof blob);
      const text = await readBlob(blob, 'text');
      return check(
        'XHR responseType blob',
        text === UTF8_TEXT && blob.type.startsWith('text/plain'),
        `${blob.type}: ${text}`,
      );
    }),
    await attempt('XHR upload of a Blob body', async () => {
      const xhr = await request('POST', `${ECHO_HTTP}/echo-body`, 'text', new Blob(['uploaded ✓']));
      return check(
        'XHR upload of a Blob body',
        xhr.responseText === 'uploaded ✓',
        xhr.responseText,
      );
    }),
    await attempt("React Native's fetch: text() in UTF-8", async () => {
      const text = await (await reactNativeFetch(`${ECHO_HTTP}/utf8`)).text();
      return check("React Native's fetch: text() in UTF-8", text === UTF8_TEXT, text);
    }),
    await attempt("React Native's fetch: blob()", async () => {
      const blob = await (await reactNativeFetch(`${ECHO_HTTP}/utf8`)).blob();
      return check(
        "React Native's fetch: blob()",
        blob.size === 17,
        `${blob.size} bytes, ${blob.type}`,
      );
    }),
    await attempt('expo/fetch: blob()', async () => {
      const blob = await (await expoFetch(`${ECHO_HTTP}/utf8`)).blob();
      const text = await readBlob(blob, 'text');
      return check(
        'expo/fetch: blob()',
        blob.size === 17 && text === UTF8_TEXT,
        `${blob.size} bytes: ${text}`,
      );
    }),
  ];
}

export function BlobScreen() {
  return <CheckList checks={useChecks('blob', blobChecks, 'modules')} />;
}
