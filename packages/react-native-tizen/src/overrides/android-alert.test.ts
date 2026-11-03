import { describe, expect, it, vi } from 'vitest';
import {
  androidAlert,
  type DialogConstants,
} from '../../react-native-overrides/Libraries/Alert/android-alert.ts';

const constants: DialogConstants = {
  buttonClicked: 'buttonClicked',
  dismissed: 'dismissed',
  buttonPositive: -1,
  buttonNegative: -2,
  buttonNeutral: -3,
};

describe('androidAlert', () => {
  it('shows a single OK button when the app gives none', () => {
    const { dialog } = androidAlert('Title', null, undefined, undefined, constants);

    expect(dialog).toEqual({
      title: 'Title',
      message: '',
      cancelable: false,
      buttonPositive: 'OK',
    });
  });

  it('makes the last of three buttons positive, then negative, then neutral', () => {
    const { dialog } = androidAlert(
      'Leave?',
      'Playback stops.',
      [{ text: 'Later' }, { text: 'Cancel' }, { text: 'Leave' }],
      { cancelable: true },
      constants,
    );

    expect(dialog).toEqual({
      title: 'Leave?',
      message: 'Playback stops.',
      cancelable: true,
      buttonNeutral: 'Later',
      buttonNegative: 'Cancel',
      buttonPositive: 'Leave',
    });
  });

  it('ignores every button past the third', () => {
    const { dialog } = androidAlert(
      't',
      'm',
      [{ text: 'a' }, { text: 'b' }, { text: 'c' }, { text: 'd' }],
      null,
      constants,
    );

    expect(dialog.buttonPositive).toBe('c');
    expect(dialog.buttonNeutral).toBe('a');
  });

  it('runs the pressed button', () => {
    const cancel = vi.fn();
    const ok = vi.fn();
    const { onAction } = androidAlert(
      't',
      'm',
      [
        { text: 'Cancel', onPress: cancel },
        { text: 'OK', onPress: ok },
      ],
      null,
      constants,
    );

    onAction('buttonClicked', -2);

    expect(cancel).toHaveBeenCalledOnce();
    expect(ok).not.toHaveBeenCalled();
  });

  it('calls onDismiss when Back closes a cancelable alert', () => {
    const onDismiss = vi.fn();
    const ok = vi.fn();
    const { onAction } = androidAlert(
      't',
      'm',
      [{ text: 'OK', onPress: ok }],
      { cancelable: true, onDismiss },
      constants,
    );

    onAction('dismissed');

    expect(onDismiss).toHaveBeenCalledOnce();
    expect(ok).not.toHaveBeenCalled();
  });
});
