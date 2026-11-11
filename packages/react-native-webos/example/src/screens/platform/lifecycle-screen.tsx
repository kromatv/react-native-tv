import { useEffect, useState } from 'react';
import { AppState, DeviceEventEmitter, Linking, View } from 'react-native';
import { EventLog, Fact, useEventLog } from './event-log.tsx';
import { RELAUNCH_EVENT, expoWebOS, type RelaunchEvent } from './webos-module.ts';

function launchParameters(): string {
  return expoWebOS ? JSON.stringify(expoWebOS.getLaunchParameters()) : 'no ExpoWebOS module';
}

export function LifecycleScreen() {
  const [entries, add] = useEventLog('lifecycle');
  const [initialUrl, setInitialUrl] = useState('asking…');
  const [parameters, setParameters] = useState(launchParameters);

  useEffect(() => {
    add(`appState ${AppState.currentState}, launched with ${launchParameters()}`);
    void Linking.getInitialURL().then((url) => {
      setInitialUrl(url ?? 'none');
      add(`getInitialURL ${url ?? 'none'}`);
    });
    const changes = AppState.addEventListener('change', (state) => add(`appState ${state}`));
    const links = Linking.addEventListener('url', ({ url }) => add(`url ${url}`));
    const relaunches = DeviceEventEmitter.addListener(RELAUNCH_EVENT, (event: RelaunchEvent) => {
      setParameters(launchParameters());
      add(`relaunch ${JSON.stringify(event.parameters)}`);
    });
    return () => {
      changes.remove();
      links.remove();
      relaunches.remove();
    };
  }, [add]);

  return (
    <View>
      <Fact label="Launch parameters" value={parameters} />
      <Fact label="Linking.getInitialURL" value={initialUrl} />
      <Fact
        label="Try"
        value="press Home, come back, launch the app again: the log says what the TV told it"
      />
      <Fact label="Back" value="leaves the app the way webOS asks: Home on a TV" />
      <EventLog entries={entries} />
    </View>
  );
}
