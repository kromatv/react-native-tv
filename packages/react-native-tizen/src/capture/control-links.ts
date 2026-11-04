import { FixableError } from '../errors.ts';
import type { ControlLink } from './control-link.ts';

/** The control links of the apps connected to one listener. */
export class ControlLinks {
  private readonly links: ControlLink[] = [];
  private readonly listeners = new Set<() => void>();

  add(link: ControlLink): void {
    this.links.push(link);
    for (const listener of this.listeners) listener();
  }

  remove(link: ControlLink): void {
    const index = this.links.indexOf(link);
    if (index !== -1) this.links.splice(index, 1);
  }

  /**
   * The newest link of `app` from `device`, either the only one connected when
   * null, waiting up to `timeoutMs` for one; null when none came. A build that
   * names no package is taken for any.
   */
  find(device: string | null, app: string | null, timeoutMs: number): Promise<ControlLink | null> {
    const found = this.pick(device, app);
    if (found !== null) return Promise.resolve(found);
    return new Promise((resolve) => {
      const check = () => {
        const link = this.pick(device, app);
        if (link === null) return;
        finish();
        resolve(link);
      };
      const timer = setTimeout(() => {
        finish();
        resolve(null);
      }, timeoutMs);
      const finish = () => {
        clearTimeout(timer);
        this.listeners.delete(check);
      };
      this.listeners.add(check);
    });
  }

  private pick(device: string | null, app: string | null): ControlLink | null {
    const matching = this.links.filter(
      (link) =>
        (device === null || link.address === device) &&
        (app === null || link.app === null || link.app === app),
    );
    const addresses = [...new Set(matching.map((link) => link.address))];
    if (addresses.length > 1) {
      throw new FixableError(
        `Apps on ${addresses.length} TVs are connected: ${addresses.join(', ')}`,
        'Pass --device <ip> to pick one.',
      );
    }
    const apps = [...new Set(matching.flatMap((link) => (link.app === null ? [] : [link.app])))];
    if (app === null && apps.length > 1) {
      throw new FixableError(
        `${apps.length} apps are connected: ${apps.join(', ')}`,
        'Pass --package <id> to pick one, or run the command in that app.',
      );
    }
    return matching.at(-1) ?? null;
  }
}
