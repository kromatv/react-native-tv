import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { testApp, type TestApp } from '../app.fixtures.ts';
import { installExpoCli } from './expo-cli.fixtures.ts';
import { fakeBun, type ManagerCall } from './fake-managers.fixtures.ts';
import { patchExpoCli } from './patch-expo-cli.ts';

let app: TestApp;
let calls: ManagerCall[];

beforeEach(() => {
  app = testApp();
  calls = [];
});

afterEach(() => {
  app.remove();
});

describe('patchExpoCli', () => {
  it('says so for an app without Expo', () => {
    app.write('bun.lock', '');

    expect(patchExpoCli(app.root, fakeBun(app, calls))).toEqual({ state: 'no-expo' });
  });

  it('says so for an entry it does not know, and changes nothing', () => {
    installExpoCli(app, 'module.exports = {};');
    app.write('package-lock.json', '');
    app.write('package.json', '{}');

    expect(patchExpoCli(app.root, fakeBun(app, calls))).toEqual({
      state: 'unknown-layout',
      version: '58.1.20',
    });
    expect(calls).toEqual([]);
  });

  it('says so for an app with no lockfile, which names no package manager', () => {
    installExpoCli(app);

    expect(patchExpoCli(app.root, fakeBun(app, calls))).toEqual({ state: 'no-lockfile' });
    expect(calls).toEqual([]);
  });
});
