/**
 * What `rn-webos` registers with: permissions only. The signed manifest other
 * clients copy from one another is refused by webOS 26, and a TV does not open
 * its pointer input socket to a client that lacks the two input permissions.
 */
export const REGISTER_MANIFEST = {
  manifestVersion: 1,
  appVersion: '1.1',
  permissions: [
    'CONTROL_INPUT_TEXT',
    'CONTROL_MOUSE_AND_KEYBOARD',
    'LAUNCH',
    'CLOSE',
    'READ_INSTALLED_APPS',
    'READ_RUNNING_APPS',
    'WRITE_NOTIFICATION_TOAST',
    'CONTROL_POWER',
    'READ_NETWORK_STATE',
  ],
} as const;
