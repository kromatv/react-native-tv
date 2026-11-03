import { describe, expect, it } from 'vitest';
import {
  bannerModel,
  installCommand,
  installProgress,
  installSucceeded,
  launchCommand,
  launchSucceeded,
  remotePackagePath,
} from './app-commands.ts';

const INSTALLED = [
  'install org.rntizen.example',
  'package_path /home/owner/share/tmp/sdk_tools/tmp/ex.tpk',
  'app_id[org.rntizen.example] install start',
  'app_id[org.rntizen.example] installing[100]',
  'app_id[org.rntizen.example] install completed',
  'spend time for wascmd is [5950]ms',
].join('\n');

describe('remotePackagePath', () => {
  it('puts the package in tmp/ under the tool path, as sdb does', () => {
    expect(remotePackagePath('/home/owner/share/tmp/sdk_tools', 'org.example-1.0.0.tpk')).toBe(
      '/home/owner/share/tmp/sdk_tools/tmp/org.example-1.0.0.tpk',
    );
  });

  it('makes the file name safe for the TV shell', () => {
    expect(remotePackagePath('/t/', '../My App;rm.tpk')).toBe('/t/tmp/My_App_rm.tpk');
  });
});

describe('commands', () => {
  it('installs with vd_appinstall and launches with execute', () => {
    expect(installCommand('org.example', '/t/tmp/a.tpk')).toBe(
      '0 vd_appinstall org.example /t/tmp/a.tpk',
    );
    expect(launchCommand('org.example')).toBe('0 execute org.example');
  });

  it('refuses an id the TV shell could misread', () => {
    expect(() => launchCommand('org.example;reboot')).toThrow('refusing');
  });
});

describe('reading the output', () => {
  it('finds the progress and the end of a successful install', () => {
    expect(installProgress('app_id[org.rntizen.example] installing[27]')).toBe(27);
    expect(installProgress('app_id[org.rntizen.example] install start')).toBeNull();
    expect(installSucceeded(INSTALLED)).toBe(true);
  });

  it('reads a failed install as a failure', () => {
    const failed =
      'app_id[org.rntizen.example] install failed[118, -12], reason: Check certificate error';

    expect(installSucceeded(failed)).toBe(false);
  });

  it('reads a launch by the pid the TV reports', () => {
    expect(launchSucceeded('... successfully launched pid = 30987 with debug 0\r\n')).toBe(true);
    expect(launchSucceeded('The app with ID: org.example is not available for the user 5001')).toBe(
      false,
    );
  });

  it('takes the model out of the banner', () => {
    expect(bannerModel('device::GQ75LS03DAUXZG::0')).toBe('GQ75LS03DAUXZG');
    expect(bannerModel('device::')).toBeNull();
    expect(bannerModel('host::')).toBeNull();
  });
});
