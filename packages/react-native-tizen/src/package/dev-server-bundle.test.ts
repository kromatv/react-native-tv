import { mkdtempSync, readFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { describe, expect, it, vi } from 'vitest';
import { Reporter } from '../cli/reporter.ts';
import { devBuildHost, devBundleUrl, devServerBundleStep } from './dev-server-bundle.ts';

describe('devBundleUrl', () => {
  it("asks Expo's dev server for its virtual entry, for the tizen platform", () => {
    expect(devBundleUrl('http://192.168.1.20:8081', '192.168.1.20')).toBe(
      'http://192.168.1.20:8081/.expo/.virtual-metro-entry.bundle?platform=tizen&dev=true&minify=false',
    );
  });

  it('replaces a loopback host the TV cannot reach', () => {
    expect(devBundleUrl('http://localhost:8081/', '192.168.5.2')).toMatch(
      /^http:\/\/192\.168\.5\.2:8081\//,
    );
    expect(devBundleUrl('http://127.0.0.1:8081', '192.168.1.20')).toMatch(
      /^http:\/\/192\.168\.1\.20:8081\//,
    );
  });

  it('refuses what is not an http origin', () => {
    expect(() => devBundleUrl('ftp://example.org', 'x')).toThrow('not a dev server address');
    expect(() => devBundleUrl('8081', 'x')).toThrow('not a dev server address');
  });
});

describe('devBuildHost', () => {
  it('reaches this computer by the log host, and refuses a release build', () => {
    expect(devBuildHost({ host: '192.168.1.20', port: 8095 })).toBe('192.168.1.20');
    expect(() => devBuildHost(null)).toThrow('A release build cannot load its bundle');
  });
});

describe('devServerBundleStep', () => {
  it('writes an empty bundle that names the dev server, and prints its URL', async () => {
    const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-dev-bundle-'));
    let printed = '';
    vi.spyOn(process.stdout, 'write').mockImplementation((chunk) => {
      printed += String(chunk);
      return true;
    });
    try {
      const reporter = new Reporter({ verbose: false, env: { CI: '1' } });

      const bundle = await devServerBundleStep(reporter, dir, 'http://localhost:8081', () =>
        Promise.resolve('192.168.1.20'),
      );
      const failing = devServerBundleStep(reporter, dir, 'http://localhost:8081', () =>
        devBuildHost(null),
      );

      await expect(failing).rejects.toThrow('A release build cannot load its bundle');

      expect(bundle.bundleUrl).toMatch(/^http:\/\/192\.168\.1\.20:8081\/\.expo\//);
      expect(readFileSync(bundle.bytecode, 'utf8')).toBe(';\n');
      expect(readFileSync(bundle.javascript, 'utf8')).toBe(';\n');
      expect(printed).toContain(bundle.bundleUrl);
    } finally {
      vi.restoreAllMocks();
      rmSync(dir, { recursive: true, force: true });
    }
  });
});
