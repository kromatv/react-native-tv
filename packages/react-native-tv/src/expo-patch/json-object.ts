/** One member of an object in JSON text, with where its key and its value sit. */
export interface JsonMember {
  readonly key: string;
  readonly keyStart: number;
  readonly keyEnd: number;
  readonly valueStart: number;
  readonly valueEnd: number;
}

/** An object in JSON text: from its `{` to after its `}`, and its members in order. */
export interface JsonObject {
  readonly start: number;
  readonly end: number;
  readonly members: readonly JsonMember[];
}

const WHITESPACE = ' \t\n\r';
const AFTER_SCALAR = `,}]${WHITESPACE}`;

function isWhitespace(text: string, at: number): boolean {
  return at < text.length && WHITESPACE.includes(text.charAt(at));
}

function skipWhitespace(text: string, from: number): number {
  let at = from;
  while (isWhitespace(text, at)) at += 1;
  return at;
}

function stringEnd(text: string, quote: number): number {
  let at = quote + 1;
  while (at < text.length && text[at] !== '"') at += text[at] === '\\' ? 2 : 1;
  return at + 1;
}

function containerEnd(text: string, open: number): number {
  let depth = 0;
  let at = open;
  while (at < text.length) {
    const char = text[at];
    if (char === '"') {
      at = stringEnd(text, at);
      continue;
    }
    if (char === '{' || char === '[') depth += 1;
    if (char === '}' || char === ']') depth -= 1;
    at += 1;
    if (depth === 0) return at;
  }
  return text.length;
}

function valueEnd(text: string, start: number): number {
  const char = text[start];
  if (char === '"') return stringEnd(text, start);
  if (char === '{' || char === '[') return containerEnd(text, start);
  let at = start;
  while (at < text.length && !AFTER_SCALAR.includes(text.charAt(at))) at += 1;
  return at;
}

/** The whitespace that ends right before `at`. */
export function whitespaceBefore(text: string, at: number): string {
  let start = at;
  while (start > 0 && isWhitespace(text, start - 1)) start -= 1;
  return text.slice(start, at);
}

/**
 * The object whose `{` is the first thing at or after `from` in `text`, which
 * must be JSON that parses, or null when a value of another kind is there.
 */
export function jsonObjectAt(text: string, from: number): JsonObject | null {
  const start = skipWhitespace(text, from);
  if (text[start] !== '{') return null;
  const members: JsonMember[] = [];
  let at = skipWhitespace(text, start + 1);
  while (text[at] === '"') {
    const keyEnd = stringEnd(text, at);
    const valueStart = skipWhitespace(text, skipWhitespace(text, keyEnd) + 1);
    const end = valueEnd(text, valueStart);
    members.push({
      key: String(JSON.parse(text.slice(at, keyEnd))),
      keyStart: at,
      keyEnd,
      valueStart,
      valueEnd: end,
    });
    at = skipWhitespace(text, end);
    if (text[at] === ',') at = skipWhitespace(text, at + 1);
  }
  return { start, end: at + 1, members };
}

/** The object at `path` below the root object of `text`, or null when there is none. */
export function jsonObjectIn(text: string, path: readonly string[]): JsonObject | null {
  let object = jsonObjectAt(text, 0);
  for (const key of path) {
    const member = object?.members.filter((candidate) => candidate.key === key).at(-1);
    if (member === undefined) return null;
    object = jsonObjectAt(text, member.valueStart);
  }
  return object;
}
