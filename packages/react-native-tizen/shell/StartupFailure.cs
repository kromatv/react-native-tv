namespace ReactNativeTizen
{
    // When the native library cannot even load, its own log stream is not there
    // yet, so the shell reports the failure to the same endpoint itself.
    internal static class StartupFailure
    {
        internal static void Report(string resourceDir, string message)
        {
            Tizen.Log.Error("ReactNative", message);
            DevLog.Write(resourceDir, "error", "The app could not start React Native: " + message);
        }
    }
}
