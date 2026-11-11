import { createInterface } from 'node:readline/promises';

/** Whether `rn-webos` can ask the user something: both ends of the terminal are a TTY. */
export function canAsk(
  input: { readonly isTTY?: boolean } = process.stdin,
  output: { readonly isTTY?: boolean } = process.stdout,
): boolean {
  return input.isTTY === true && output.isTTY === true;
}

/** Asks `question` on the terminal; an empty answer takes `fallback`, which is shown unless empty. */
export async function ask(
  question: string,
  fallback: string,
  input: NodeJS.ReadableStream = process.stdin,
  output: NodeJS.WritableStream = process.stdout,
): Promise<string> {
  const prompt = createInterface({ input, output });
  try {
    const shown = fallback === '' ? '' : ` (${fallback})`;
    const answer = (await prompt.question(`${question}${shown} `)).trim();
    return answer === '' ? fallback : answer;
  } finally {
    prompt.close();
  }
}
