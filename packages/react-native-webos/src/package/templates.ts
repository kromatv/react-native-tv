import type { WebOSAppConfig } from '../config/webos-config.ts';
import type { PlatformProfile } from '../webos/platform-profiles.ts';
import { applicationExtras, previewServiceAppId, projectReferences } from './feature-manifest.ts';
import { escapeXml } from './xml.ts';

const PLACEHOLDER = /\{\{(\w+)\}\}/g;

/**
 * Fills every `{{name}}` in a template. A placeholder without a value throws,
 * so a template and the code that fills it cannot drift apart.
 */
export function renderTemplate(template: string, values: Readonly<Record<string, string>>): string {
  return template.replace(PLACEHOLDER, (_match, name: string) => {
    const value = values[name];
    if (value === undefined) throw new Error(`No value for the template placeholder {{${name}}}`);
    return value;
  });
}

/** The values `shell/webos-manifest.xml` is filled with, escaped for XML. */
export function manifestValues(
  config: WebOSAppConfig,
  profile: PlatformProfile,
): Record<string, string> {
  return {
    package: escapeXml(config.packageId),
    version: escapeXml(config.version),
    apiVersion: profile.apiVersion,
    label: escapeXml(config.label),
    applicationExtras: applicationExtras(config.features),
    privileges: config.privileges
      .map((privilege) => `    <privilege>${escapeXml(privilege)}</privilege>`)
      .join('\n'),
  };
}

/** The values `shell/preview-service/webos-manifest.xml` is filled with. */
export function serviceManifestValues(
  config: WebOSAppConfig,
  profile: PlatformProfile,
): Record<string, string> {
  return {
    package: escapeXml(config.packageId),
    version: escapeXml(config.version),
    apiVersion: profile.apiVersion,
    label: escapeXml(`${config.label} preview`),
    serviceAppId: escapeXml(previewServiceAppId(config.packageId)),
  };
}

/** The values `shell/ReactNativeWebOS.csproj` and the preview service's project are filled with. */
export function projectValues(
  profile: PlatformProfile,
  config: Pick<WebOSAppConfig, 'features'>,
): Record<string, string> {
  return {
    targetFramework: profile.targetFramework,
    webosNetVersion: profile.webosNetVersion,
    projectReferences: projectReferences(config.features),
  };
}
