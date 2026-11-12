import { Alert, View, useTVEventHandler } from 'react-native';
import { EventLog, Fact, useEventLog } from './event-log.tsx';

function askToDelete(add: (entry: string) => void) {
  Alert.alert(
    'Delete the recording?',
    'This cannot be undone.',
    [
      { text: 'Later', onPress: () => add('Later') },
      { text: 'Cancel', style: 'cancel', onPress: () => add('Cancel') },
      { text: 'Delete', style: 'destructive', onPress: () => add('Delete') },
    ],
    { cancelable: true, onDismiss: () => add('dismissed with Back') },
  );
}

function confirmSaved(add: (entry: string) => void) {
  Alert.alert('Saved', 'Back does not close this one.', [
    { text: 'Undo', onPress: () => add('Undo') },
    { text: 'OK', onPress: () => add('OK') },
  ]);
}

function throwLater() {
  setTimeout(() => {
    throw new Error('The example threw this on purpose');
  }, 0);
}

export function AlertsScreen() {
  const [entries, add] = useEventLog('alerts');

  useTVEventHandler((event) => {
    if (event.eventKeyAction !== 1) return;
    if (event.eventType === 'select') askToDelete(add);
    if (event.eventType === 'up') confirmSaved(add);
    if (event.eventType === 'down') throwLater();
    if (event.eventType === 'playPause') console.error('The example logged this error on purpose');
  });

  return (
    <View>
      <Fact label="Enter" value="an alert with three buttons, which Back dismisses" />
      <Fact label="Up" value="an alert with two buttons" />
      <Fact label="Down" value="throws: the red screen, or LogBox with a dev server" />
      <Fact label="Play/Pause" value="logs an error: LogBox with a dev server" />
      <EventLog entries={entries} />
    </View>
  );
}
