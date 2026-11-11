import { fetchText } from './http-text.ts';
import { SECOND_SCREEN_SEARCH_TARGET } from './ssdp.ts';

/** What a UPnP device description says a device is. */
export interface UpnpDevice {
  readonly friendlyName: string;
  readonly manufacturer: string;
  /** The model. An LG TV's `modelName` is the constant `LG Smart TV`. */
  readonly modelNumber: string;
  /** The UUID of `UDN`, without its `uuid:` prefix; empty when there is none. */
  readonly udn: string;
  /** Whether the device offers LG's remote control service. */
  readonly secondScreen: boolean;
}

const MAX_DESCRIPTION_BYTES = 32 * 1024;
const UDN = /^uuid:([0-9A-Za-z-]{1,64})$/;
const ENTITIES: Record<string, string> = {
  '&quot;': '"',
  '&apos;': "'",
  '&#39;': "'",
  '&lt;': '<',
  '&gt;': '>',
  '&amp;': '&',
};

/** Undoes the HTML escaping a TV puts in the name its owner typed: `Tom&apos;s TV`. */
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
    modelNumber: elementText(xml, 'modelNumber'),
    udn: UDN.exec(elementText(xml, 'UDN'))?.[1] ?? '',
    secondScreen: xml.includes(`<serviceType>${SECOND_SCREEN_SEARCH_TARGET}</serviceType>`),
  };
}

/** Whether a description is an LG TV's: LG made it and it offers the remote control service. */
export function isLgTv(device: UpnpDevice): boolean {
  return device.manufacturer.startsWith('LG') && device.secondScreen;
}

/** The description an SSDP reply points at, or null when it does not answer or names nothing. */
export async function fetchDeviceDescription(location: string): Promise<UpnpDevice | null> {
  const xml = await fetchText(location, MAX_DESCRIPTION_BYTES);
  if (xml === null) return null;
  const device = parseDeviceDescription(xml);
  return device.friendlyName === '' && device.manufacturer === '' ? null : device;
}
