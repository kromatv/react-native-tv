import type { TvFeatures } from '../config/tv-features.ts';
import { escapeXml } from './xml.ts';

/** The directory, inside the generated project, of the preview service app. */
export const PREVIEW_SERVICE_DIR = 'PreviewService';

const BASE_SCREEN_RESOLUTION = 'http://tizen.org/metadata/app_ui_type/base_screen_resolution';
const USE_PREVIEW = 'http://samsung.com/tv/metadata/use.preview';
const EDEN_RESUME = 'http://samsung.com/appcontrol/operation/eden_resume';

/** The app id of the service app that refreshes a personal preview. */
export function previewServiceAppId(packageId: string): string {
  return `${packageId}.preview`;
}

/** Whether the package carries the preview service app. */
export function hasPreviewService(features: TvFeatures): boolean {
  return features.preview?.kind === 'personal';
}

function metadata(key: string, value: string): string {
  return `<metadata key="${escapeXml(key)}" value="${escapeXml(value)}" />`;
}

function appControls(features: TvFeatures): string[] {
  if (!hasPreviewService(features)) return [];
  return [`<app-control>\n      <operation name="${EDEN_RESUME}" />\n    </app-control>`];
}

function metadataLines(features: TvFeatures): string[] {
  const lines: string[] = [];
  if (features.resolution === '4k') lines.push(metadata(BASE_SCREEN_RESOLUTION, '4k_only'));
  if (features.preview?.kind === 'public') {
    lines.push(metadata(USE_PREVIEW, `endpoint_URL=${features.preview.endpoint}`));
  }
  if (features.preview?.kind === 'personal') lines.push(metadata(USE_PREVIEW, 'bg_service'));
  return lines;
}

/**
 * What the features add inside the `ui-application` element, in the order the
 * manifest schema expects: app controls, metadata, background category.
 */
export function applicationExtras(features: TvFeatures): string {
  const lines = [
    ...appControls(features),
    ...metadataLines(features),
    ...(features.backgroundMedia ? ['<background-category value="media" />'] : []),
  ];
  return lines.map((line) => `\n    ${line}`).join('');
}

/** The project reference that packs the preview service app into the TPK. */
export function projectReferences(features: TvFeatures): string {
  if (!hasPreviewService(features)) return '';
  return `\n    <ProjectReference Include="${PREVIEW_SERVICE_DIR}/PreviewService.csproj" />`;
}
