import type { TvStartProvider } from './tv-start-registry.ts';

const ESCAPES: Readonly<Record<string, string>> = {
  '&': '&amp;',
  '<': '&lt;',
  '>': '&gt;',
  '"': '&quot;',
  "'": '&#39;',
};

function escapeHtml(text: string): string {
  return text.replaceAll(/[&<>"']/g, (character) => ESCAPES[character] ?? character);
}

/** The page of this package in Expo's tools menu: each TV platform of the app, linked to its own page. */
export function platformsPage(providers: readonly TvStartProvider[]): string {
  const items = providers.map(
    (provider) =>
      `<li><a href="/_expo/plugins/${escapeHtml(provider.package)}/">${escapeHtml(provider.platform)}</a> ${escapeHtml(provider.package)}</li>`,
  );
  const list =
    items.length === 0
      ? '<p>No TV platform package of this app has a page here.</p>'
      : `<ul>${items.join('')}</ul>`;
  return `<!doctype html><html lang="en"><meta charset="utf-8"><title>TV platforms</title><h1>TV platforms of this app</h1>${list}<p>In the terminal of expo start, press t to open the app on a TV and shift+t to choose one.</p></html>`;
}
