import { describe, expect, it } from 'vitest';
import { parseArpTable } from './arp.ts';
import { compareHosts, localSubnets, prefixOf, subnetHosts } from './lan.ts';
import { mapPool } from './map-pool.ts';
import { parseSsdpReply, searchRequest } from './ssdp.ts';
import { isLgTv, parseDeviceDescription } from './upnp.ts';
import { webosVersionName } from './webos-version.ts';

const SECOND_SCREEN = 'urn:lge-com:service:webos-second-screen:1';
const UUID = '6f2c1c1e-8b0a-4c57-9d0e-2a7b3a4f5e61';

describe('ssdp', () => {
  it("writes the M-SEARCH of UPnP Device Architecture 1.1 for LG's remote control service", () => {
    expect(searchRequest(SECOND_SCREEN).toString()).toBe(
      `M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: "ssdp:discover"\r\nMX: 2\r\nST: ${SECOND_SCREEN}\r\n\r\n`,
    );
  });

  it('reads the answer of an LG TV: where its description is, its UUID and what it answered for', () => {
    const reply = [
      'HTTP/1.1 200 OK',
      'LOCATION: http://192.168.1.50:1702/',
      'CACHE-CONTROL: max-age=1800',
      'EXT:',
      `USN: uuid:${UUID}::${SECOND_SCREEN}`,
      `ST: ${SECOND_SCREEN}`,
      'DATE: Sun, 17 Jan 2016 13:52:40 GMT',
      'SERVER: WebOS/4.1.0 UPnP/1.0',
      '',
    ].join('\r\n');

    expect(parseSsdpReply(reply, '192.168.1.50')).toEqual({
      host: '192.168.1.50',
      location: 'http://192.168.1.50:1702/',
      server: 'WebOS/4.1.0 UPnP/1.0',
      target: SECOND_SCREEN,
      uuid: UUID,
    });
  });

  it('refuses a LOCATION that is not HTTP, and reads no UUID from a USN without one', () => {
    expect(parseSsdpReply('HTTP/1.1 200 OK\r\nLOCATION: file:///etc/passwd\r\n', 'x')).toBeNull();
    expect(
      parseSsdpReply('HTTP/1.1 200 OK\r\nLOCATION: http://x/\r\nUSN: upnp:rootdevice\r\n', 'x'),
    ).toMatchObject({ uuid: null, target: '' });
  });
});

describe('upnp', () => {
  const description = `<root xmlns="urn:schemas-upnp-org:device-1-0" xmlns:dlna="urn:schemas-dlna-org:device-1-0">
  <specVersion><major>1</major><minor>0</minor></specVersion>
  <device>
    <deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>
    <friendlyName>[LG] webOS TV OLED65B4PUA</friendlyName>
    <manufacturer>LG Electronics</manufacturer>
    <manufacturerURL>http://www.lge.com</manufacturerURL>
    <modelDescription/>
    <modelName>LG Smart TV</modelName>
    <modelURL>http://www.lge.com</modelURL>
    <modelNumber>OLED65B4PUA</modelNumber>
    <serialNumber/>
    <UDN>uuid:${UUID}</UDN>
    <serviceList>
      <service>
        <serviceType>${SECOND_SCREEN}</serviceType>
        <serviceId>urn:lge-com:serviceId:webos-second-screen-3000-3001</serviceId>
      </service>
    </serviceList>
  </device>
</root>`;

  it('reads the name, the model number and the UUID of an LG TV', () => {
    const device = parseDeviceDescription(description);

    expect(device).toEqual({
      friendlyName: '[LG] webOS TV OLED65B4PUA',
      manufacturer: 'LG Electronics',
      modelNumber: 'OLED65B4PUA',
      udn: UUID,
      secondScreen: true,
    });
    expect(isLgTv(device)).toBe(true);
  });

  it('unescapes the name its owner typed, and tells another device from a TV', () => {
    const renderer =
      '<root><device><friendlyName>Tom&apos;s TV</friendlyName><manufacturer>LG Electronics</manufacturer></device></root>';

    const device = parseDeviceDescription(renderer);

    expect(device.friendlyName).toBe("Tom's TV");
    expect(isLgTv(device)).toBe(false);
    expect(isLgTv({ ...parseDeviceDescription(description), manufacturer: 'Acme' })).toBe(false);
  });
});

describe('webosVersionName', () => {
  it('names the releases LG markets by year, and leaves the others as they are', () => {
    expect(webosVersionName('11.2.0')).toBe('webOS 26 (11.2.0)');
    expect(webosVersionName('7.3.0')).toBe('webOS 22 (7.3.0)');
    expect(webosVersionName('4.4.2')).toBe('webOS 4.4.2');
    expect(webosVersionName('12.0.0')).toBe('webOS 12.0.0');
  });
});

describe('lan', () => {
  it('reads the /24 of an IPv4 address only', () => {
    expect(prefixOf('192.168.1.42')).toBe('192.168.1');
    expect(prefixOf('192.168.1.420')).toBeNull();
    expect(prefixOf('fe80::1')).toBeNull();
  });

  it('lists a /24 without this machine, and sorts addresses by number', () => {
    const hosts = subnetHosts('10.0.0', '10.0.0.5');

    expect(hosts).toHaveLength(253);
    expect(hosts).not.toContain('10.0.0.5');
    expect(['10.0.0.20', '10.0.0.3'].sort(compareHosts)).toEqual(['10.0.0.3', '10.0.0.20']);
  });

  it('skips loopback, IPv6 and virtual interfaces', () => {
    const entry = (address: string, family: 'IPv4' | 'IPv6', internal = false) => ({
      address,
      family,
      internal,
    });
    const interfaces = {
      lo0: [entry('127.0.0.1', 'IPv4', true)],
      en0: [entry('192.168.1.124', 'IPv4'), entry('fe80::1', 'IPv6')],
      bridge100: [entry('192.168.64.1', 'IPv4')],
    };

    expect(localSubnets(interfaces)).toEqual([
      { iface: 'en0', address: '192.168.1.124', prefix: '192.168.1' },
    ]);
  });
});

describe('parseArpTable', () => {
  it('keeps the complete entries of `arp -an`', () => {
    const output = [
      '? (192.168.1.1) at 0:11:22:33:44:55 on en0 ifscope [ethernet]',
      '? (192.168.1.107) at 0:11:22:33:44:66 on en0 ifscope [ethernet]',
      '? (192.168.1.200) at (incomplete) on en0 ifscope [ethernet]',
    ].join('\n');

    expect(parseArpTable(output)).toEqual(['192.168.1.1', '192.168.1.107']);
  });
});

describe('mapPool', () => {
  it('keeps the input order and the concurrency limit', async () => {
    let running = 0;
    let most = 0;
    const results = await mapPool([30, 10, 20, 5], 2, async (ms) => {
      running += 1;
      most = Math.max(most, running);
      await new Promise((resolve) => setTimeout(resolve, ms));
      running -= 1;
      return ms * 2;
    });

    expect(results).toEqual([60, 20, 40, 10]);
    expect(most).toBe(2);
  });
});
