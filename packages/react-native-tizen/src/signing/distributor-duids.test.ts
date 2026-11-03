import { describe, expect, it } from 'vitest';
import { distributorDuids } from './distributor-duids.ts';

describe('distributorDuids', () => {
  it('reads the device ids a Samsung distributor certificate lists', () => {
    const subjectAltName =
      'URI:URN:tizen:packageid=, URI:URN:tizen:deviceid=ABCDEFGH12345, URI:URN:tizen:deviceid=ZYXWVUT987654';

    expect(distributorDuids({ subjectAltName })).toEqual(['ABCDEFGH12345', 'ZYXWVUT987654']);
  });

  it('finds none in a certificate without subject alternative names', () => {
    expect(distributorDuids({ subjectAltName: undefined })).toEqual([]);
  });
});
