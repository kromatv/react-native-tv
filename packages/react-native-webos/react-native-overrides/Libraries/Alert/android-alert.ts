/** A button of `Alert.alert`, as React Native's `AlertButton`. */
export interface AlertButton {
  readonly text?: string;
  readonly onPress?: ((value?: string) => unknown) | null;
  readonly isPreferred?: boolean;
  readonly style?: 'default' | 'cancel' | 'destructive';
}

/** The options of `Alert.alert` an Android dialog reads. */
export interface AlertOptions {
  readonly cancelable?: boolean | null;
  readonly onDismiss?: (() => void) | null;
}

/** `DialogManagerAndroid.getConstants()`. */
export interface DialogConstants {
  readonly buttonClicked: string;
  readonly dismissed: string;
  readonly buttonPositive: number;
  readonly buttonNegative: number;
  readonly buttonNeutral: number;
}

/** What `DialogManagerAndroid.showAlert` takes. */
export interface DialogOptions {
  title: string;
  message: string;
  cancelable: boolean;
  buttonPositive?: string;
  buttonNegative?: string;
  buttonNeutral?: string;
}

/** The dialog to show, and what to run when it reports an action. */
export interface AndroidAlert {
  readonly dialog: DialogOptions;
  readonly onAction: (action: string, buttonKey?: number) => void;
}

const DEFAULT_POSITIVE_TEXT = 'OK';

/**
 * Lays `Alert.alert`'s arguments out as React Native does on Android: at most
 * three buttons, the last one positive, then negative, then neutral, and a
 * dialog that Back closes only when `cancelable` is set.
 */
export function androidAlert(
  title: string | null | undefined,
  message: string | null | undefined,
  buttons: readonly AlertButton[] | null | undefined,
  options: AlertOptions | null | undefined,
  constants: DialogConstants,
): AndroidAlert {
  const dialog: DialogOptions = {
    title: title ?? '',
    message: message ?? '',
    cancelable: options?.cancelable === true,
  };
  const valid = buttons ? buttons.slice(0, 3) : [{ text: DEFAULT_POSITIVE_TEXT }];
  const positive = valid.pop();
  const negative = valid.pop();
  const neutral = valid.pop();
  if (neutral) dialog.buttonNeutral = neutral.text ?? '';
  if (negative) dialog.buttonNegative = negative.text ?? '';
  if (positive) dialog.buttonPositive = positive.text || DEFAULT_POSITIVE_TEXT;

  const onAction = (action: string, buttonKey?: number): void => {
    if (action === constants.dismissed) {
      options?.onDismiss?.();
      return;
    }
    if (action !== constants.buttonClicked) return;
    if (buttonKey === constants.buttonNeutral) neutral?.onPress?.();
    else if (buttonKey === constants.buttonNegative) negative?.onPress?.();
    else if (buttonKey === constants.buttonPositive) positive?.onPress?.();
  };
  return { dialog, onAction };
}
