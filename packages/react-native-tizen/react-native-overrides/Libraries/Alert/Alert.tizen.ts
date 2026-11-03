import NativeDialogManagerAndroid from 'react-native/Libraries/NativeModules/specs/NativeDialogManagerAndroid';
import { androidAlert, type AlertButton, type AlertOptions } from './android-alert.ts';

/**
 * React Native's `Alert` on Tizen. `Alert.js` shows a dialog only when
 * `Platform.OS` is `ios` or `android`; Tizen takes Android's path, a native
 * dialog from `DialogManagerAndroid`. `prompt` is iOS only, as on Android.
 */
class Alert {
  static alert(
    title?: string | null,
    message?: string | null,
    buttons?: readonly AlertButton[] | null,
    options?: AlertOptions | null,
  ): void {
    if (!NativeDialogManagerAndroid) return;
    const { dialog, onAction } = androidAlert(
      title,
      message,
      buttons,
      options,
      NativeDialogManagerAndroid.getConstants(),
    );
    NativeDialogManagerAndroid.showAlert(dialog, (error: string) => console.warn(error), onAction);
  }

  static prompt(): void {}
}

export default Alert;
