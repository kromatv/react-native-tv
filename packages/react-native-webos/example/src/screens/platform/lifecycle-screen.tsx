import { useEffect } from 'react';
import { AppState, Linking, View } from 'react-native';
import { EventLog, Fact, useEventLog } from './event-log.tsx';

export function LifecycleScreen() {
  const [entries, add] = useEventLog('lifecycle');

  useEffect(() => {
    add(`appState ${AppState.currentState}`);
    const changes = AppState.addEventListener('change', (state) => add(`appState ${state}`));
    const links = Linking.addEventListener('url', ({ url }) => add(`url ${url}`));
    return () => {
      changes.remove();
      links.remove();
    };
  }, [add]);

  return (
    <View>
      <Fact label="Try" value="press Home, wait, come back: the log says what changed" />
      <Fact label="Deep link" value="open the app with a URL: the log shows it" />
      <EventLog entries={entries} />
    </View>
  );
}
