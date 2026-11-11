import type { X509Certificate } from 'node:crypto';

const DEVICE_ID = /URI:URN:tizen:deviceid=([A-Za-z0-9]+)/g;

/** The DUIDs a distributor certificate lists in its subject alternative names: the TVs it installs on. */
export function distributorDuids(certificate: Pick<X509Certificate, 'subjectAltName'>): string[] {
  return [...(certificate.subjectAltName ?? '').matchAll(DEVICE_ID)].flatMap((match) =>
    match[1] === undefined ? [] : [match[1]],
  );
}
