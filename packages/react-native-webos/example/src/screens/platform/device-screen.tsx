import { I18nManager, PixelRatio, Platform, ScrollView, useWindowDimensions } from 'react-native';
import { Fact } from './event-log.tsx';
import { expoWebOS } from './webos-module.ts';

function size(value: { readonly width: number; readonly height: number } | undefined): string {
  return value ? `${value.width}x${value.height}` : 'no ExpoWebOS module';
}

const constants = new Map<string, unknown>(Object.entries(Platform.constants));

function constant(name: string): string {
  return String(constants.get(name) ?? 'missing');
}

export function DeviceScreen() {
  const window = useWindowDimensions();
  return (
    <ScrollView>
      <Fact label="Platform.OS, isTV" value={`${Platform.OS}, ${String(Platform.isTV)}`} />
      <Fact label="Platform.Version" value={String(Platform.Version)} />
      <Fact label="constants.Release" value={constant('Release')} />
      <Fact label="constants.Model" value={constant('Model')} />
      <Fact label="constants.Fingerprint" value={constant('Fingerprint')} />
      <Fact
        label="constants.Brand, Manufacturer"
        value={`${constant('Brand')}, ${constant('Manufacturer')}`}
      />
      <Fact label="constants.uiMode" value={constant('uiMode')} />
      <Fact
        label="ExpoWebOS model, webOS, firmware"
        value={
          expoWebOS
            ? `${expoWebOS.modelName}, ${expoWebOS.platformVersion}, ${expoWebOS.buildString}`
            : 'no ExpoWebOS module'
        }
      />
      <Fact label="Menu language" value={expoWebOS?.locale ?? 'no ExpoWebOS module'} />
      <Fact
        label="I18nManager.localeIdentifier"
        value={I18nManager.getConstants().localeIdentifier ?? 'none'}
      />
      <Fact
        label="Intl default locale"
        value={new Intl.DateTimeFormat().resolvedOptions().locale}
      />
      <Fact
        label="A date in that locale"
        value={new Date(Date.UTC(2026, 10, 11, 20, 30)).toLocaleString()}
      />
      <Fact label="Surface, in pixels" value={size(expoWebOS?.getScreenSize())} />
      <Fact label="Panel, in pixels" value={size(expoWebOS?.getPanelSize())} />
      <Fact
        label="Window, in points"
        value={`${window.width}x${window.height} at ${PixelRatio.get()}`}
      />
    </ScrollView>
  );
}
