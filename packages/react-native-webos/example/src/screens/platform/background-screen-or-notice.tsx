import { requireOptionalNativeModule } from 'expo';
import { Suspense, lazy } from 'react';
import { Fact } from './event-log.tsx';

// expo-video throws at import in a runtime built without it.
const BackgroundScreen = lazy(async () => {
  const { BackgroundScreen: Screen } = await import('./background-screen.tsx');
  return { default: Screen };
});

export function BackgroundScreenOrNotice() {
  if (requireOptionalNativeModule('ExpoVideo') === null) {
    return <Fact label="Background audio" value="this runtime was built without expo-video" />;
  }
  return (
    <Suspense fallback={null}>
      <BackgroundScreen />
    </Suspense>
  );
}
