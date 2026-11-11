const HEADER_SIZE = 32;

function dataHeader(format: string): Uint8Array {
  const header = new Uint8Array(HEADER_SIZE);
  const view = new DataView(header.buffer);
  view.setUint16(0, HEADER_SIZE, true);
  header.set([0xda, 0x27], 2);
  view.setUint16(4, 20, true);
  header[10] = 2;
  header.set(new TextEncoder().encode(format), 12);
  header[16] = 2;
  return header;
}

/** An ICU common data header, for files `writeIcuCommonData` writes in tests. */
export function commonDataHeader(): Uint8Array {
  return dataHeader('CmnD');
}

/**
 * A `res_index.res` as genrb writes it in format 2: a root table holding
 * `InstalledLocales`, a 16-bit table of `locales` to empty strings.
 */
export function resIndex(locales: readonly string[]): Uint8Array {
  const keys = ['InstalledLocales', ...locales];
  const keyBytes = new TextEncoder().encode(keys.map((key) => `${key}\0`).join(''));
  const keysStart = 32;
  const keysTop = Math.ceil((keysStart + keyBytes.byteLength) / 4) * 4;
  const keyOffsets = keys.map(
    (_, index) =>
      keysStart + keys.slice(0, index).reduce((total, key) => total + key.length + 1, 0),
  );
  const units = [0, locales.length, ...keyOffsets.slice(1), ...locales.map(() => 0)];
  const unitsEnd = keysTop + Math.ceil((units.length * 2) / 4) * 4;
  const body = new DataView(new ArrayBuffer(unitsEnd + 8));
  body.setUint32(0, (2 << 28) | (unitsEnd / 4), true);
  body.setUint32(4, 7, true);
  body.setUint32(8, keysTop / 4, true);
  new Uint8Array(body.buffer).set(keyBytes, keysStart);
  units.forEach((unit, index) => body.setUint16(keysTop + index * 2, unit, true));
  body.setUint16(unitsEnd, 1, true);
  body.setUint16(unitsEnd + 2, keyOffsets[0] ?? 0, true);
  body.setUint32(unitsEnd + 4, (5 << 28) | 1, true);
  const file = new Uint8Array(HEADER_SIZE + body.byteLength);
  file.set(dataHeader('ResB'), 0);
  file.set(new Uint8Array(body.buffer), HEADER_SIZE);
  return file;
}
