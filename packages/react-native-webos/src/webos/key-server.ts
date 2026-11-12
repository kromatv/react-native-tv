import { createPrivateKey } from 'node:crypto';
import { FixableError } from '../errors.ts';

/** The port of the HTTP server the Developer Mode app starts for Key Server. */
export const KEY_SERVER_PORT = 9991;

const FETCH_TIMEOUT_MS = 5000;
const MAX_KEY_BYTES = 65_536;
const PEM_KEY = /-----BEGIN [A-Z ]*PRIVATE KEY-----/;

const KEY_SERVER_HINT =
  'Open the Developer Mode app on the TV and turn Key Server on. It turns off again when the TV restarts.';
const PASSPHRASE_HINT =
  'It is the six characters at the bottom left of the Developer Mode app, upper and lower case as shown.';

async function bodyText(response: Response): Promise<string | null> {
  if (response.body === null) return null;
  const chunks: Uint8Array[] = [];
  let size = 0;
  for await (const chunk of response.body) {
    size += chunk.byteLength;
    if (size > MAX_KEY_BYTES) return null;
    chunks.push(chunk);
  }
  return Buffer.concat(chunks).toString('utf8');
}

/** Fetches the passphrase-protected SSH key the TV at `host` serves while Key Server is on. */
export async function fetchDeveloperKey(host: string, port = KEY_SERVER_PORT): Promise<string> {
  const tv = port === KEY_SERVER_PORT ? host : `${host}:${port}`;
  let response: Response;
  try {
    response = await fetch(`http://${host}:${port}/webos_rsa`, {
      signal: AbortSignal.timeout(FETCH_TIMEOUT_MS),
    });
  } catch {
    throw new FixableError(`The key server of the TV at ${tv} does not answer`, KEY_SERVER_HINT);
  }
  const pem = response.status === 200 ? await bodyText(response) : null;
  if (pem === null || !PEM_KEY.test(pem)) {
    const what =
      response.status === 200 ? 'something that is not a key' : `HTTP ${response.status}`;
    throw new FixableError(`The key server of the TV at ${tv} answered ${what}`, KEY_SERVER_HINT);
  }
  return pem;
}

/**
 * Opens the key a TV served with the passphrase its Developer Mode app shows,
 * and returns it unprotected, as the PKCS#1 PEM an SSH client reads.
 */
export function decryptDeveloperKey(pem: string, passphrase: string): string {
  let key;
  try {
    key = createPrivateKey({ key: pem, passphrase });
  } catch {
    throw new FixableError('The passphrase does not open the key the TV served', PASSPHRASE_HINT);
  }
  if (key.asymmetricKeyType !== 'rsa') {
    throw new FixableError(
      `The TV served a ${key.asymmetricKeyType ?? 'key of an unknown type'}, not the RSA key rn-webos expects`,
      'Report it with the TV model and webOS version: https://github.com/kromatv/react-native-webos/issues',
    );
  }
  return key.export({ type: 'pkcs1', format: 'pem' }).toString();
}
