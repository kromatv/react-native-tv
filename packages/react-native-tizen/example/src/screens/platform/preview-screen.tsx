import { SmartHubPreview, type PreviewSection } from '@kromatv/react-native-tizen/preview';
import { useEffect, useState } from 'react';
import { Linking, View, useTVEventHandler } from 'react-native';
import { EventLog, Fact, useEventLog } from './event-log.tsx';

const poster = (seed: string) => `https://picsum.photos/seed/${seed}/640/360.jpg`;

const SAMPLE: readonly PreviewSection[] = [
  {
    title: 'Continue watching',
    tiles: [
      {
        title: 'Big Buck Bunny',
        subtitle: '4 min left',
        image: poster('rntizen-1'),
        ratio: '16:9',
        url: 'rntizen-example://tile/1',
        playable: true,
      },
      {
        title: 'Sintel',
        subtitle: '12 min left',
        image: poster('rntizen-2'),
        ratio: '16:9',
        url: 'rntizen-example://tile/2',
        playable: true,
      },
    ],
  },
  {
    title: 'New',
    tiles: [
      {
        title: 'Tears of Steel',
        image: poster('rntizen-3'),
        ratio: '16:9',
        url: 'rntizen-example://tile/3',
      },
      {
        title: 'Elephants Dream',
        image: poster('rntizen-4'),
        ratio: '16:9',
        url: 'rntizen-example://tile/4',
      },
    ],
  },
];

export function PreviewScreen() {
  const [entries, add] = useEventLog('preview');
  const [initialUrl, setInitialUrl] = useState<string | null>(null);

  useEffect(() => {
    void Linking.getInitialURL().then((url) => {
      setInitialUrl(url);
      if (url !== null) add(`Linking initial URL ${url}`);
    });
    const subscription = Linking.addEventListener('url', (event) =>
      add(`Linking url ${event.url}`),
    );
    return () => subscription.remove();
  }, [add]);

  useTVEventHandler((event) => {
    if (event.eventKeyAction !== 1) return;
    if (event.eventType === 'select') {
      add('publishing 4 tiles');
      void SmartHubPreview.set(SAMPLE).then((result) => add(`set: ${JSON.stringify(result)}`));
    }
    if (event.eventType === 'playPause') {
      void SmartHubPreview.clear().then((result) => add(`clear: ${JSON.stringify(result)}`));
    }
  });

  return (
    <View>
      <Fact label="SmartHubPreview.isAvailable" value={String(SmartHubPreview.isAvailable)} />
      <Fact label="launchedFromTile" value={String(SmartHubPreview.launchedFromTile)} />
      <Fact label="Linking.getInitialURL()" value={initialUrl ?? 'null'} />
      <Fact label="Enter" value="publishes 4 tiles; Play/Pause clears them" />
      <Fact label="Back on this app" value="exits at once after a tile launch, asks otherwise" />
      <EventLog entries={entries} />
    </View>
  );
}
