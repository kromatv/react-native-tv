import { createInterface } from 'node:readline/promises';

/** Whether `rn-tizen` can ask the user something: both ends of the terminal are a TTY. */
export function canAsk(
  input: { readonly isTTY?: boolean } = process.stdin,
  output: { readonly isTTY?: boolean } = process.stdout,
): boolean {
  return input.isTTY === true && output.isTTY === true;
}

/** Asks `question` on the terminal; an empty answer takes `fallback`. */
export async function ask(
  question: string,
  fallback: string,
  input: NodeJS.ReadableStream = process.stdin,
  output: NodeJS.WritableStream = process.stdout,
): Promise<string> {
  const prompt = createInterface({ input, output });
  try {
    const answer = (await prompt.question(`${question} (${fallback}) `)).trim();
    return answer === '' ? fallback : answer;
  } finally {
    prompt.close();
  }
}
