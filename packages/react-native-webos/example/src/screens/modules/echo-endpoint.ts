declare const process: { readonly env: { readonly EXPO_PUBLIC_MODULES_ECHO?: string } };

/**
 * The machine running `scripts/modules-echo-server.ts`, as the app reaches it:
 * the simulator VM's name for the Mac unless `EXPO_PUBLIC_MODULES_ECHO` was
 * set when the app was bundled.
 */
export const ECHO_HTTP = process.env.EXPO_PUBLIC_MODULES_ECHO ?? 'http://host.lima.internal:47100';

export const ECHO_SOCKET = `${ECHO_HTTP.replace(/^http/, 'ws')}/socket`;

export const UTF8_TEXT = 'héllo wörld ✓';
