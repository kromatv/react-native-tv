/** What a package's `control` file says that differs from one package to the next. */
export interface ControlFields {
  readonly packageId: string;
  readonly version: string;
  readonly architecture: string;
  /** Bytes, not Debian's kibibytes: the TV compares it with the free space. */
  readonly installedSize: number;
}

/**
 * The `control` file of a webOS package, the ten lines `ares-package` writes.
 * A package without the two `webOS-` fields is called hand-rolled by
 * webosbrew's verifier (docs/reference/webos-ipk-format.md, section 2).
 */
export function controlFile(fields: ControlFields): string {
  const lines = [
    `Package: ${fields.packageId}`,
    `Version: ${fields.version}`,
    'Section: misc',
    'Priority: optional',
    `Architecture: ${fields.architecture}`,
    `Installed-Size: ${fields.installedSize}`,
    'Maintainer: N/A <nobody@example.com>',
    'Description: This is a webOS application.',
    'webOS-Package-Format-Version: 2',
    'webOS-Packager-Version: x.y.x',
  ];
  return `${lines.join('\n')}\n`;
}

/** The fields of a `control` file, each line split at its first colon as the TV's installer splits it. */
export function parseControl(control: string): Map<string, string> {
  const fields = new Map<string, string>();
  for (const line of control.split('\n')) {
    const colon = line.indexOf(':');
    if (colon > 0) fields.set(line.slice(0, colon).trim(), line.slice(colon + 1).trim());
  }
  return fields;
}
