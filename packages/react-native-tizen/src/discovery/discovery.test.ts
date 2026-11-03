import { describe, expect, it } from 'vitest';
import { parseArpTable } from './arp.ts';
import { compareHosts, localSubnets, prefixOf, subnetHosts } from './lan.ts';
import { mapPool } from './map-pool.ts';
import { parseApiDevice } from './samsung-api.ts';
import { parseSsdpReply, searchRequest } from './ssdp.ts';
import { parseDeviceDescription } from './upnp.ts';

describe('ssdp', () => {
  it('writes the M-SEARCH of UPnP Device Architecture 1.1', () => {
    expect(searchRequest('urn:dial-multiscreen-org:service:dial:1').toString()).toBe(
      'M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: "ssdp:discover"\r\nMX: 2\r\nST: urn:dial-multiscreen-org:service:dial:1\r\n\r\n',
    );
  });

  it('reads where a Samsung TV keeps its description', () => {
    const reply = [
      'HTTP/1.1 200 OK',
      'LOCATION: http://192.168.1.107:7678/nservice/',
      'SERVER: Samsung-Linux/4.1, UPnP/1.0, Samsung_UPnP_SDK/1.0',
      'ST: urn:dial-multiscreen-org:service:dial:1',
      '',
    ].join('\r\n');

    expect(parseSsdpReply(reply, '192.168.1.107')).toEqual({
      host: '192.168.1.107',
      location: 'http://192.168.1.107:7678/nservice/',
      server: 'Samsung-Linux/4.1, UPnP/1.0, Samsung_UPnP_SDK/1.0',
    });
    expect(parseSsdpReply('HTTP/1.1 200 OK\r\nLOCATION: file:///etc/passwd\r\n', 'x')).toBeNull();
  });
});

describe('upnp', () => {
  it('reads the name its owner gave the TV, unescaped', () => {
    const xml =
      '<root><device><friendlyName>75&quot; The Frame</friendlyName><manufacturer>Samsung</manufacturer><modelName>GQ75LS03DAUXZG</modelName></device></root>';

    expect(parseDeviceDescription(xml)).toEqual({
      friendlyName: '75" The Frame',
      manufacturer: 'Samsung',
      modelName: 'GQ75LS03DAUXZG',
    });
  });
});

describe('parseApiDevice', () => {
  it('reads a TV in Developer mode and the host PC it names', () => {
    const device = {
      OS: 'Tizen',
      name: '75&quot; The Frame',
      model: '24_PTM_FTV_T09',
      modelName: 'GQ75LS03DAUXZG',
      developerMode: '1',
      developerIP: '192.168.1.124',
    };

    expect(parseApiDevice({ device })).toEqual({
      name: '75" The Frame',
      modelName: 'GQ75LS03DAUXZG',
      developerMode: true,
      developerIp: '192.168.1.124',
    });
  });

  it('leaves out a soundbar, which answers the same API', () => {
    const device = {
      OS: 'Tizen',
      name: 'Soundbar',
      model: '25_MT8532_AISPK',
      ModelNumber: 'HW-QS700F',
      developerMode: '0',
    };

    expect(parseApiDevice({ device })).toBeNull();
  });

  it('reads no host PC from an empty or zero address', () => {
    const device = { OS: 'Tizen', developerMode: '0', developerIP: '0.0.0.0' };

    expect(parseApiDevice({ device })?.developerIp).toBeNull();
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
