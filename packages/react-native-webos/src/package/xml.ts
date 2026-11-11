const XML_ENTITIES: Record<string, string> = {
  '&': '&amp;',
  '<': '&lt;',
  '>': '&gt;',
  '"': '&quot;',
  "'": '&apos;',
};

/** Escapes text for an XML element or attribute. */
export function escapeXml(text: string): string {
  return text.replace(/[&<>"']/g, (character) => XML_ENTITIES[character] ?? character);
}
