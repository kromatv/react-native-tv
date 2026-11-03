import type { SignatureRole, SignedFile } from './signature-xml.ts';

export const AUTHOR_SIGNATURE = 'author-signature.xml';
const DISTRIBUTOR_SIGNATURE = /^signature[1-9]\d*\.xml$/;
const ESCAPED = new Set('% <>#{}|\\^~[]`;/?@=&$!*:+');
const XML_ATTRIBUTE: Record<string, string> = {
  '"': '&quot;',
  '\t': '&#x9;',
  '\n': '&#xA;',
  '\r': '&#xD;',
};

/** A distributor signature's file name: `signature<n>.xml`, n counting from 1. */
export function distributorSignatureName(index: number): string {
  return `signature${index + 1}.xml`;
}

/** Whether a package path is one of its signatures, which live at its root. */
export function isSignatureFile(path: string): boolean {
  return path === AUTHOR_SIGNATURE || DISTRIBUTOR_SIGNATURE.test(path);
}

const percent = (bytes: Buffer) =>
  [...bytes].map((byte) => `%${byte.toString(16).toUpperCase().padStart(2, '0')}`).join('');

/** A package path as a `Reference URI`: Tizen's reserved characters and non-ASCII percent-escaped. */
export function referenceUri(path: string): string {
  let uri = '';
  for (const character of path) {
    const code = character.codePointAt(0) ?? 0;
    if (code > 0x7f || ESCAPED.has(character)) uri += percent(Buffer.from(character, 'utf8'));
    else uri += XML_ATTRIBUTE[character] ?? character;
  }
  return uri;
}

/**
 * The files a signature covers, in the order Tizen lists them: every file but
 * the distributor signatures, and for the author its own signature too,
 * sorted by UTF-16 code unit.
 */
export function signedFiles(files: ReadonlyMap<string, Buffer>, role: SignatureRole): SignedFile[] {
  return [...files.keys()]
    .filter(
      (path) =>
        !DISTRIBUTOR_SIGNATURE.test(path) && !(role === 'author' && path === AUTHOR_SIGNATURE),
    )
    .sort((left, right) => (left < right ? -1 : 1))
    .map((path) => ({ uri: referenceUri(path), bytes: files.get(path) ?? Buffer.alloc(0) }));
}
