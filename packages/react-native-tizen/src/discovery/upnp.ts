import { fetchText } from './http-text.ts';

/** What a UPnP device description says a device is. */
export interface UpnpDevice {
  readonly friendlyName: string;
  readonly manufacturer: string;
  readonly modelName: string;
}

const MAX_DESCRIPTION_BYTES = 32 * 1024;
const ENTITIES: Record<string, string> = {
  '&quot;': '"',
  '&apos;': "'",
  '&#39;': "'",
  '&lt;': '<',
  '&gt;': '>',
  '&amp;': '&',
};

/** Undoes the HTML escaping a TV puts in the name its owner typed: `75&quot; The Frame`. */
export function unescapeName(text: string): string {
  return text.replace(/&(?:quot|apos|#39|lt|gt|amp);/g, (entity) => ENTITIES[entity] ?? entity);
}

function elementText(xml: string, tag: string): string {
  const open = xml.indexOf(`<${tag}>`);
  if (open === -1) return '';
  const start = open + tag.length + 2;
  const end = xml.indexOf('</', start);
  return end === -1 ? '' : unescapeName(xml.slice(start, end).trim());
}

export function parseDeviceDescription(xml: string): UpnpDevice {
  return {
    friendlyName: elementText(xml, 'friendlyName'),
    manufacturer: elementText(xml, 'manufacturer'),
    modelName: elementText(xml, 'modelName'),
  };
}

/** The description an SSDP reply points at, or null when it does not answer or names nothing. */
export async function fetchDeviceDescription(location: string): Promise<UpnpDevice | null> {
  const xml = await fetchText(location, MAX_DESCRIPTION_BYTES);
  if (xml === null) return null;
  const device = parseDeviceDescription(xml);
  return device.friendlyName === '' && device.manufacturer === '' ? null : device;
}
