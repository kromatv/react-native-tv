import { describe, expect, it } from 'vitest';
import { listedPlugins } from './listed-plugins.ts';

describe('listedPlugins', () => {
  it('names the plugins listed as a string or as an entry, and skips the rest', () => {
    const plugins = ['expo-font', ['expo-video', { a: 1 }], ['alone'], [() => null, {}], 3, []];

    expect([...listedPlugins(plugins)]).toEqual(['expo-font', 'expo-video', 'alone']);
  });

  it('names none for a config with no list of plugins', () => {
    expect(listedPlugins(undefined).size).toBe(0);
    expect(listedPlugins({ 'expo-font': true }).size).toBe(0);
  });
});
