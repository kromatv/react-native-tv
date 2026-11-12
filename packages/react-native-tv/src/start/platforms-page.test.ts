import { describe, expect, it } from 'vitest';
import { platformsPage } from './platforms-page.ts';
import { fakeProvider } from './provider.fixtures.ts';

describe('platformsPage', () => {
  it("links each platform to its package's page", () => {
    const page = platformsPage([fakeProvider('tizen', []), fakeProvider('webos', [])]);

    expect(page).toContain('<li><a href="/_expo/plugins/@tv/tizen/">tizen</a> @tv/tizen</li>');
    expect(page).toContain('<li><a href="/_expo/plugins/@tv/webos/">webos</a> @tv/webos</li>');
  });

  it('says so when no platform has a page', () => {
    expect(platformsPage([])).toContain('No TV platform package of this app has a page here.');
  });

  it('escapes what a package calls itself', () => {
    const page = platformsPage([{ ...fakeProvider('x', []), package: `"><script>'&` }]);

    expect(page).toContain('&quot;&gt;&lt;script&gt;&#39;&amp;');
    expect(page).not.toContain('<script>');
  });
});
