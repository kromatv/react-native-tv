import { useEffect, useState } from 'react';
import { Linking, View, useTVEventHandler } from 'react-native';
import { EventLog, Fact, useEventLog } from './event-log.tsx';

const WEB_URL = 'https://www.lg.com';
const APP_URL = 'example://title/42';

function reason(error: unknown): string {
  return error instanceof Error ? error.message : String(error);
}

export function DeepLinkScreen() {
  const [entries, add] = useEventLog('deep-link');
  const [initialUrl, setInitialUrl] = useState('asking…');
  const [lastUrl, setLastUrl] = useState('none yet');
  const [canOpen, setCanOpen] = useState('asking…');

  useEffect(() => {
    void Linking.getInitialURL().then((url) => setInitialUrl(url ?? 'none'));
    void Promise.all([Linking.canOpenURL(WEB_URL), Linking.canOpenURL(APP_URL)]).then(
      ([web, app]) => setCanOpen(`${WEB_URL} ${String(web)}, ${APP_URL} ${String(app)}`),
    );
    const links = Linking.addEventListener('url', ({ url }) => {
      setLastUrl(url);
      add(`url ${url}`);
    });
    return () => links.remove();
  }, [add]);

  useTVEventHandler((event) => {
    if (event.eventKeyAction !== 1 || event.eventType !== 'select') return;
    add(`openURL ${WEB_URL}`);
    Linking.openURL(WEB_URL).then(
      () => add('openURL resolved: the browser opens'),
      (error: unknown) => add(`openURL rejected: ${reason(error)}`),
    );
  });

  return (
    <View>
      <Fact label="Launched with" value={initialUrl} />
      <Fact label="Last url event" value={lastUrl} />
      <Fact label="Linking.canOpenURL" value={canOpen} />
      <Fact label="Enter" value={`Linking.openURL(${WEB_URL}), in the TV's browser`} />
      <Fact label="From outside" value={`launch the app with {"contentTarget":"${APP_URL}"}`} />
      <EventLog entries={entries} />
    </View>
  );
}
