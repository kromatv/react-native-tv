import { useRef, useState } from 'react';
import {
  StyleSheet,
  Text,
  TextInput,
  type TextInputProps,
  View,
  useTVEventHandler,
} from 'react-native';
import { useEventLog } from './use-event-log.ts';

interface FieldSpec {
  readonly name: string;
  readonly label: string;
  readonly props: TextInputProps;
}

const FIELDS: readonly FieldSpec[] = [
  {
    name: 'search',
    label: 'Search, autoFocus',
    props: {
      placeholder: 'Rechercher',
      returnKeyType: 'search',
      autoFocus: true,
      autoCorrect: false,
    },
  },
  {
    name: 'password',
    label: 'Password, maxLength 8',
    props: {
      placeholder: 'Mot de passe',
      secureTextEntry: true,
      maxLength: 8,
      autoCapitalize: 'none',
    },
  },
  {
    name: 'centered',
    label: 'Centered, keeps focus on submit',
    props: {
      placeholder: 'Centré',
      placeholderTextColor: '#ffb300',
      textAlign: 'center',
      submitBehavior: 'submit',
      selectionColor: '#3d5afe',
    },
  },
  {
    name: 'multiline',
    label: 'Multiline',
    props: { multiline: true, placeholder: 'Plusieurs lignes', style: { height: 132 } },
  },
  {
    name: 'readonly',
    label: 'Not editable',
    props: { editable: false, defaultValue: 'Lecture seule' },
  },
];

/**
 * Every TextInput behaviour a TV needs: Up and Down move a ring over the
 * fields, OK focuses the ringed one (the on-screen keyboard opens on a TV),
 * typing goes to the focused one, Back closes the keyboard and then leaves the
 * field. The events each field sends are listed on the right.
 */
export function TextInputScreen() {
  const [ring, setRing] = useState(0);
  const [editing, setEditing] = useState<string | null>(null);
  const [upper, setUpper] = useState('');
  const [log, addLog] = useEventLog();
  const inputs = useRef<Record<string, TextInput | null>>({});

  useTVEventHandler((event) => {
    if (event.eventKeyAction !== 1) return;
    if (event.eventType === 'down') setRing((index) => Math.min(FIELDS.length, index + 1));
    if (event.eventType === 'up') setRing((index) => Math.max(0, index - 1));
    if (event.eventType === 'select') {
      const name = ring < FIELDS.length ? FIELDS[ring]?.name : 'upper';
      if (name) inputs.current[name]?.focus();
    }
  });

  const handlers = (name: string): TextInputProps => ({
    onFocus: () => {
      setEditing(name);
      setRing(name === 'upper' ? FIELDS.length : FIELDS.findIndex((field) => field.name === name));
      addLog(`${name} focus`);
    },
    onBlur: () => {
      setEditing((current) => (current === name ? null : current));
      addLog(`${name} blur`);
    },
    onEndEditing: (event) => addLog(`${name} endEditing "${event.nativeEvent.text}"`),
    onChange: (event) =>
      addLog(`${name} change #${event.nativeEvent.eventCount} "${event.nativeEvent.text}"`),
    onSelectionChange: (event) =>
      addLog(
        `${name} selection ${event.nativeEvent.selection.start}-${event.nativeEvent.selection.end}`,
      ),
    onSubmitEditing: (event) => addLog(`${name} submit "${event.nativeEvent.text}"`),
    onKeyPress: (event) => addLog(`${name} key ${event.nativeEvent.key}`),
  });

  return (
    <View style={styles.screen}>
      <View style={styles.form}>
        <Text style={styles.title}>TextInput</Text>
        <Text style={styles.hint}>
          {editing ? `Typing in ${editing}` : 'Up, Down, then OK to type'}
        </Text>
        {FIELDS.map((field, index) => (
          <View key={field.name} style={[styles.field, ring === index && styles.ringed]}>
            <Text style={styles.label}>{field.label}</Text>
            <TextInput
              ref={(input) => {
                inputs.current[field.name] = input;
              }}
              {...field.props}
              {...handlers(field.name)}
              style={[styles.input, field.props.style]}
            />
          </View>
        ))}
        <View style={[styles.field, ring === FIELDS.length && styles.ringed]}>
          <Text style={styles.label}>Controlled, uppercased by JavaScript</Text>
          <TextInput
            ref={(input) => {
              inputs.current.upper = input;
            }}
            style={styles.input}
            value={upper}
            onChangeText={(text) => setUpper(text.toUpperCase())}
            placeholder="MAJUSCULES"
            {...handlers('upper')}
          />
        </View>
      </View>
      <View style={styles.log}>
        <Text style={styles.label}>Events, newest first</Text>
        {log.map((line) => (
          <Text key={line.id} style={styles.logLine} numberOfLines={1}>
            {line.text}
          </Text>
        ))}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, flexDirection: 'row', backgroundColor: '#0b0f1a', padding: 56, gap: 48 },
  form: { width: 980 },
  title: { color: '#ffffff', fontSize: 44, fontWeight: '700' },
  hint: { color: '#a9b4cf', fontSize: 22, marginTop: 4, marginBottom: 16 },
  field: {
    borderRadius: 14,
    borderWidth: 3,
    borderColor: 'transparent',
    padding: 8,
    marginBottom: 6,
  },
  ringed: { borderColor: '#ffb300' },
  label: { color: '#a9b4cf', fontSize: 20, marginBottom: 6 },
  input: {
    backgroundColor: '#1a2236',
    color: '#ffffff',
    fontSize: 28,
    borderRadius: 10,
    paddingHorizontal: 18,
    height: 60,
  },
  log: { flex: 1, backgroundColor: '#121829', borderRadius: 16, padding: 24 },
  logLine: { color: '#ffffff', fontSize: 20, marginTop: 6 },
});
