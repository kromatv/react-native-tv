import { createSign, type X509Certificate } from 'node:crypto';
import type { Pkcs12Contents } from './pkcs12.ts';
import type { SigningKey } from './signature-xml.ts';

/** The key of a PKCS#12 file with its chain ordered leaf first, each certificate followed by its issuer. */
export function signingKeyFrom({ key, certificates }: Pkcs12Contents): SigningKey {
  const leaf = certificates.find((certificate) => certificate.checkPrivateKey(key));
  if (leaf === undefined)
    throw new Error('PKCS#12: no certificate in the file matches its private key');
  const chain: X509Certificate[] = [leaf];
  const rest = certificates.filter((certificate) => certificate !== leaf);
  for (let last = leaf; last.subject !== last.issuer;) {
    const next = rest.findIndex((certificate) => certificate.subject === last.issuer);
    const [issuer] = next === -1 ? [] : rest.splice(next, 1);
    if (issuer === undefined) break;
    chain.push(issuer);
    last = issuer;
  }
  return { chain, sign: (data) => createSign('RSA-SHA512').update(data).sign(key) };
}
