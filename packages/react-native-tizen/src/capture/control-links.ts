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
   * The newest link from `device`, or from the only TV connected when it is
   * null, waiting up to `timeoutMs` for one; null when none came.
   */
  find(device: string | null, timeoutMs: number): Promise<ControlLink | null> {
    const found = this.pick(device);
    if (found !== null) return Promise.resolve(found);
    return new Promise((resolve, reject) => {
      const check = () => {
        try {
          const link = this.pick(device);
          if (link === null) return;
          finish();
          resolve(link);
        } catch (error) {
          finish();
          reject(error);
        }
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

  private pick(device: string | null): ControlLink | null {
    const matching =
      device === null ? this.links : this.links.filter((link) => link.address === device);
    const addresses = [...new Set(matching.map((link) => link.address))];
    if (addresses.length > 1) {
      throw new FixableError(
        `Apps on ${addresses.length} TVs are connected: ${addresses.join(', ')}`,
        'Pass --device <ip> to pick one.',
      );
    }
    return matching.at(-1) ?? null;
  }
}
