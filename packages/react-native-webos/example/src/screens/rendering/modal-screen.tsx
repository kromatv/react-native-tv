import { useState } from 'react';
import { Modal, StyleSheet, Text, View, useTVEventHandler } from 'react-native';
import { ScreenFrame, palette } from './screen-frame.tsx';

export function ModalScreen() {
  const [visible, setVisible] = useState(false);
  const [shown, setShown] = useState(0);
  const [closeRequests, setCloseRequests] = useState(0);
  useTVEventHandler((event) => {
    if (event.eventType === 'select') setVisible((value) => !value);
  });
  return (
    <ScreenFrame
      title="Modal"
      hint="Enter: open or close the modal · Back closes it through onRequestClose"
    >
      <Text
        style={styles.status}
      >{`onShow fired ${shown} times · onRequestClose ${closeRequests} times`}</Text>
      <View style={styles.clipped}>
        <Text style={styles.note}>The modal is declared inside this clipped, translucent box.</Text>
        <Modal
          transparent
          visible={visible}
          onShow={() => setShown((count) => count + 1)}
          onRequestClose={() => {
            console.log('modal: onRequestClose');
            setCloseRequests((count) => count + 1);
            setVisible(false);
          }}
        >
          <View style={styles.backdrop}>
            <View style={styles.dialog}>
              <Text style={styles.dialogTitle}>A modal</Text>
              <Text style={styles.note}>Drawn above everything, across the logical screen.</Text>
            </View>
          </View>
        </Modal>
      </View>
    </ScreenFrame>
  );
}

const styles = StyleSheet.create({
  status: { color: palette.amber, fontSize: 26 },
  clipped: {
    width: 600,
    height: 200,
    marginTop: 32,
    overflow: 'hidden',
    opacity: 0.6,
    backgroundColor: palette.card,
    padding: 24,
  },
  note: { color: palette.muted, fontSize: 26 },
  backdrop: {
    flex: 1,
    backgroundColor: 'rgba(0, 0, 0, 0.6)',
    alignItems: 'center',
    justifyContent: 'center',
  },
  dialog: {
    width: 800,
    backgroundColor: '#24304d',
    borderRadius: 32,
    padding: 48,
    borderWidth: 2,
    borderColor: palette.amber,
  },
  dialogTitle: { color: palette.text, fontSize: 44, fontWeight: '700', marginBottom: 16 },
});
