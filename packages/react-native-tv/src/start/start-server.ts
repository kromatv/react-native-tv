import { platformsPage } from './platforms-page.ts';
import { startTvKeys } from './start-tv-keys.ts';
import { tvStartProviders, tvStartRegistry } from './tv-start-registry.ts';

startTvKeys();

/**
 * This package's dev tools plugin server inside `expo start`. Expo loads the
 * module as its dev server starts, which is when the keys are added; the one
 * page it serves lists the TV platforms of the app.
 */
export default function handler(request: Request): Response | undefined {
  const { pathname } = new URL(request.url);
  if (pathname !== '/' && pathname !== '') return undefined;
  return new Response(platformsPage(tvStartProviders(tvStartRegistry())), {
    headers: { 'content-type': 'text/html; charset=utf-8', 'cache-control': 'no-store' },
  });
}
