using System;
using Tizen.Applications;

namespace ReactNativeTizen
{
    internal static class PreviewService
    {
        internal const string RefreshOperation = "http://react-native-tizen/appcontrol/operation/preview_refresh";

        internal static void Refresh(string serviceAppId)
        {
            try
            {
                var request = new AppControl { ApplicationId = serviceAppId, Operation = RefreshOperation };
                AppControl.SendLaunchRequest(request);
            }
            catch (Exception error)
            {
                NativeHost.Warn("Smart Hub preview: cannot start " + serviceAppId + ": " + error.Message);
            }
        }
    }
}
