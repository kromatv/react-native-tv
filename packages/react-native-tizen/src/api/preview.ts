import { TurboModuleRegistry, type TurboModule } from 'react-native';
import { createPublishQueue, type PublishResult } from './publish-queue.ts';
import { toSamsungPreview, type PreviewSection } from './preview-json.ts';
import { parsePreviewSource, type PreviewSource } from './preview-source.ts';

export type { PublishResult } from './publish-queue.ts';
export type { PreviewSource } from './preview-source.ts';
export {
  MAX_PREVIEW_TILES,
  PreviewValidationError,
  toSamsungPreview,
  type PreviewSection,
  type PreviewTile,
  type TileRatio,
} from './preview-json.ts';

interface NativePreview extends TurboModule {
  isAvailable(): boolean;
  launchedFromTile(): boolean;
  publish(json: string): Promise<string>;
  setSource(json: string | null): void;
}

const native = TurboModuleRegistry.get<NativePreview>('RNTizenSmartHubPreview');

const NO_SMART_HUB: PublishResult = {
  published: false,
  reason:
    'no personal Smart Hub preview: run on a Samsung TV with expo.tizen.preview.refreshMinutes set',
};

const publishQueue = createPublishQueue(async (json) => {
  if (!native?.isAvailable()) return NO_SMART_HUB;
  try {
    return { published: true, channel: await native.publish(json) };
  } catch (error) {
    return { published: false, reason: error instanceof Error ? error.message : String(error) };
  }
});

/**
 * The tiles Samsung's home screen shows above the app while it is highlighted.
 * Needs `expo.tizen.preview` with `refreshMinutes`; elsewhere every call
 * resolves with `published: false`.
 */
export const SmartHubPreview = {
  /** Whether the app can publish: on a Samsung TV, with `expo.tizen.preview.refreshMinutes` set. */
  get isAvailable(): boolean {
    return native?.isAvailable() ?? false;
  },
  /**
   * Whether the app was opened from one of its tiles. Samsung asks that Back on
   * the first screen then exits with no confirmation.
   */
  get launchedFromTile(): boolean {
    return native?.launchedFromTile() ?? false;
  },
  /**
   * Checks the sections against Samsung's rules (throwing a
   * `PreviewValidationError`) and publishes them. Samsung takes one update per
   * 10 minutes: a call inside that window waits, and the last one wins.
   */
  set(sections: readonly PreviewSection[]): Promise<PublishResult> {
    return publishQueue(JSON.stringify(toSamsungPreview(sections)));
  },
  /** Empties the preview. */
  clear(): Promise<PublishResult> {
    return publishQueue(JSON.stringify({ sections: [] }));
  },
  /**
   * Where the preview service fetches tiles while the app is closed, Samsung's
   * JSON as `toSamsungPreview` writes it; null stops fetching.
   */
  setSource(source: PreviewSource | null): void {
    native?.setSource(source === null ? null : JSON.stringify(parsePreviewSource(source)));
  },
};
