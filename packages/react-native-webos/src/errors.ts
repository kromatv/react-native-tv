/**
 * An expected failure the user can do something about. The CLI prints the
 * message and the hint, never a stack trace.
 */
export class FixableError extends Error {
  readonly hint: string;

  constructor(message: string, hint: string) {
    super(message);
    this.name = 'FixableError';
    this.hint = hint;
  }
}
