/** A signing profile from Tizen Studio's Certificate Manager. */
export interface SecurityProfile {
  readonly name: string;
  readonly active: boolean;
}

const ACTIVE_MARK = 'O';

/**
 * Parses `tizen security-profiles list`, a fixed-width table whose `[Active]`
 * column holds an `O` on the active profile's row.
 */
export function parseSecurityProfiles(output: string): SecurityProfile[] {
  const lines = output.split(/\r?\n/);
  const header = lines.findIndex((line) => line.includes('[Profile Name]'));
  if (header === -1) return [];
  const activeColumn = lines[header]?.indexOf('[Active]') ?? -1;
  const profiles: SecurityProfile[] = [];
  for (const line of lines.slice(header + 1)) {
    if (line.trim() === '') continue;
    const name = (activeColumn > 0 ? line.slice(0, activeColumn) : line).trim();
    const mark = activeColumn > 0 ? line.slice(activeColumn).trim() : '';
    if (name !== '') profiles.push({ name, active: mark === ACTIVE_MARK });
  }
  return profiles;
}
