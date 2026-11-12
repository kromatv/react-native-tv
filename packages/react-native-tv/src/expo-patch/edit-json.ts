import { jsonObjectAt, jsonObjectIn, whitespaceBefore, type JsonObject } from './json-object.ts';

interface MemberStyle {
  readonly lead: string;
  readonly colon: string;
}

const TWO_SPACES: MemberStyle = { lead: '\n  ', colon: ': ' };

function styleOf(text: string, object: JsonObject): MemberStyle | null {
  const last = object.members.at(-1);
  if (last === undefined) return null;
  return {
    lead: whitespaceBefore(text, last.keyStart),
    colon: text.slice(last.keyEnd, last.valueStart),
  };
}

function oneLevelDeeper(style: MemberStyle): MemberStyle {
  const newline = style.lead.lastIndexOf('\n');
  return newline === -1
    ? style
    : { lead: style.lead + style.lead.slice(newline + 1), colon: style.colon };
}

function withMember(
  text: string,
  object: JsonObject,
  member: string,
  style: MemberStyle,
  beforeClose: string,
): string {
  const last = object.members.at(-1);
  return last === undefined
    ? `${text.slice(0, object.start + 1)}${style.lead}${member}${beforeClose}${text.slice(object.end - 1)}`
    : `${text.slice(0, last.valueEnd)},${style.lead}${member}${text.slice(last.valueEnd)}`;
}

/**
 * `text`, the JSON of an object, without the member at `path`, and without each
 * object on the way that the removal leaves empty. Everything else keeps its
 * formatting. A path that names no member gives `text` back.
 */
export function removeJsonMember(text: string, path: readonly string[]): string {
  const object = jsonObjectIn(text, path.slice(0, -1));
  if (object === null) return text;
  const index = object.members.map((member) => member.key).lastIndexOf(path.at(-1) ?? '');
  const member = object.members[index];
  if (member === undefined) return text;
  const next = object.members[index + 1];
  if (next !== undefined) return text.slice(0, member.keyStart) + text.slice(next.keyStart);
  const previous = object.members[index - 1];
  if (previous !== undefined) return text.slice(0, previous.valueEnd) + text.slice(member.valueEnd);
  return path.length > 1
    ? removeJsonMember(text, path.slice(0, -1))
    : text.slice(0, object.start + 1) + text.slice(object.end - 1);
}

/**
 * `text`, the JSON of an object, with `key` of its member object `holder` set
 * to the string `value`. The member and its object are added, in the
 * formatting of their neighbours, when they are not there. Null when `text`
 * has no root object or `holder` is not an object.
 */
export function setJsonString(
  text: string,
  holder: string,
  key: string,
  value: string,
): string | null {
  const root = jsonObjectAt(text, 0);
  if (root === null) return null;
  const rootStyle = styleOf(text, root) ?? TWO_SPACES;
  const inner = oneLevelDeeper(rootStyle);
  const held = root.members.filter((member) => member.key === holder).at(-1);
  const written = (style: MemberStyle) =>
    `${JSON.stringify(key)}${style.colon}${JSON.stringify(value)}`;
  if (held === undefined) {
    const object = `{${inner.lead}${written(inner)}${rootStyle.lead}}`;
    const member = `${JSON.stringify(holder)}${rootStyle.colon}${object}`;
    return withMember(text, root, member, rootStyle, '\n');
  }
  const object = jsonObjectAt(text, held.valueStart);
  if (object === null) return null;
  const existing = object.members.filter((member) => member.key === key).at(-1);
  if (existing !== undefined) {
    return (
      text.slice(0, existing.valueStart) + JSON.stringify(value) + text.slice(existing.valueEnd)
    );
  }
  const style = styleOf(text, object) ?? inner;
  return withMember(text, object, written(style), style, rootStyle.lead);
}
