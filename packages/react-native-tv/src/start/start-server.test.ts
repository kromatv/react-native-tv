import { describe, expect, it } from 'vitest';
import handler from './start-server.ts';

describe('the dev tools plugin server', () => {
  it('serves the page of the TV platforms at its root, and nothing else', async () => {
    const page = handler(new Request('http://localhost:8081/'));

    expect(page?.headers.get('content-type')).toBe('text/html; charset=utf-8');
    expect(await page?.text()).toContain('<h1>TV platforms of this app</h1>');
    expect(handler(new Request('http://localhost:8081/api/jobs'))).toBeUndefined();
  });
});
