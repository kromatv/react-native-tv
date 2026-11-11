const ENTITIES: Record<string, string> = {
  '&amp;': '&',
  '&lt;': '<',
  '&gt;': '>',
  '&quot;': '"',
  '&apos;': "'",
};

/** The `name="value"` attributes in the text of one tag, entities decoded. */
export function attributes(text: string): Record<string, string> {
  const values: Record<string, string> = {};
  let rest = text;
  for (let at = rest.indexOf('="'); at !== -1; at = rest.indexOf('="')) {
    const end = rest.indexOf('"', at + 2);
    if (end === -1) break;
    const name = rest.slice(0, at).trim().split(/\s+/).at(-1) ?? '';
    values[name] = rest
      .slice(at + 2, end)
      .replace(/&(?:amp|lt|gt|quot|apos);/g, (entity) => ENTITIES[entity] ?? entity);
    rest = rest.slice(end + 1);
  }
  return values;
}

/** The attributes of the first `<tag ...>` in `xml`, or null when there is none. */
export function firstTagAttributes(xml: string, tag: string): Record<string, string> | null {
  for (let at = xml.indexOf(`<${tag}`); at !== -1; at = xml.indexOf(`<${tag}`, at + 1)) {
    const next = xml[at + tag.length + 1];
    if (next !== ' ' && next !== '\n' && next !== '\t' && next !== '\r') continue;
    const end = xml.indexOf('>', at);
    return end === -1 ? null : attributes(xml.slice(at + tag.length + 1, end));
  }
  return null;
}
