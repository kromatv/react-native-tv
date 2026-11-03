import { X509Certificate } from 'node:crypto';
import { describe, expect, it } from 'vitest';
import { testSigner } from './signing.fixtures.ts';

describe('issueCertificate', () => {
  it('issues a leaf its CA signed, and a CA that may issue', () => {
    const { key, chain } = testSigner('Distributor');
    const [leaf, ca] = chain;

    expect(leaf?.verify(ca?.publicKey ?? leaf.publicKey)).toBe(true);
    expect(leaf?.checkPrivateKey(key)).toBe(true);
    expect(leaf?.ca).toBe(false);
    expect(ca?.ca).toBe(true);
    expect(leaf?.issuer).toBe(ca?.subject);
    expect(leaf?.subject).toBe('C=CH\nCN=Distributor Test Signer');
    expect(ca).toBeInstanceOf(X509Certificate);
  });
});
