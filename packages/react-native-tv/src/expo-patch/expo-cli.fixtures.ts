import type { TestApp } from '../app.fixtures.ts';

/** The shape of the entry of Expo CLI 58: its command table, then its argument parser. */
export const EXPO_CLI_ENTRY = `const commands = {
    start: ()=>import('../src/start/index.js').then((i)=>i.expoStart)
};
const args = (0, _arg().default)({
    '--help': Boolean
});
`;

export const EXPO_CLI_ENTRY_FILE = 'node_modules/@expo/cli/build/src/index.js';

export function installExpoCli(app: TestApp, entry: string | null = EXPO_CLI_ENTRY): void {
  app.write('node_modules/expo/package.json', { name: 'expo', version: '58.0.0' });
  app.write('node_modules/@expo/cli/package.json', {
    name: '@expo/cli',
    version: '58.1.20',
    exports: { '.': './build/src/index.js', './package.json': './package.json' },
  });
  if (entry !== null) app.write(EXPO_CLI_ENTRY_FILE, entry);
}
