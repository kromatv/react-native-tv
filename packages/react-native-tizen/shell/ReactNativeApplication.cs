using System;
using Tizen.Applications;

namespace ReactNativeTizen
{
    // Samsung's store takes web and .NET apps only, so this shell is the .NET
    // app; React Native itself, window included, lives in the native library.
    internal sealed class ReactNativeApplication : CoreUIApplication
    {
        private static NativeHost.PreviewPublisher previewPublisher;

        private IntPtr host = IntPtr.Zero;
        private bool launched;
        private ShellConfig config;
        private VoiceChannel voice;

        protected override void OnCreate()
        {
            base.OnCreate();
            try
            {
                host = NativeHost.rn_tizen_host_create(DirectoryInfo.Resource);
            }
            catch (Exception error)
            {
                StartupFailure.Report(DirectoryInfo.Resource, error.ToString());
            }
            if (host == IntPtr.Zero)
            {
                Exit();
                return;
            }
            config = ShellConfig.Read(DirectoryInfo.Resource);
            if (config.PersonalPreview)
            {
                var packageId = ApplicationInfo.PackageId;
                previewPublisher = json => PreviewPublisher.Publish(json, packageId, NativeHost.Info);
                NativeHost.rn_tizen_host_set_preview_publisher(host, previewPublisher);
            }
            if (config.Voice)
            {
                voice = VoiceChannel.Start(host);
                NativeHost.rn_tizen_host_set_voice_available(host, voice != null ? 1 : 0);
            }
        }

        protected override void OnPause()
        {
            base.OnPause();
            if (host != IntPtr.Zero) NativeHost.rn_tizen_host_pause(host);
            if (config?.PreviewService != null) PreviewService.Refresh(config.PreviewService);
        }

        protected override void OnResume()
        {
            base.OnResume();
            if (host != IntPtr.Zero) NativeHost.rn_tizen_host_resume(host);
        }

        protected override void OnAppControlReceived(AppControlReceivedEventArgs e)
        {
            base.OnAppControlReceived(e);
            var control = e.ReceivedAppControl;
            var isLaunch = !launched;
            launched = true;
            if (host == IntPtr.Zero) return;
            TilePayload.LogExtras(control);
            var tileUrl = TilePayload.Url(control);
            NativeHost.rn_tizen_host_set_launched_from_tile(host, tileUrl != null ? 1 : 0);
            var uri = tileUrl ?? control.Uri;
            if (string.IsNullOrEmpty(uri)) return;
            if (isLaunch) NativeHost.rn_tizen_host_set_initial_url(host, uri);
            else NativeHost.rn_tizen_host_open_url(host, uri);
        }

        protected override void OnTerminate()
        {
            voice?.Dispose();
            voice = null;
            if (host != IntPtr.Zero) NativeHost.rn_tizen_host_destroy(host);
            host = IntPtr.Zero;
            base.OnTerminate();
        }

        private static void Main(string[] args)
        {
            NativeHost.rn_tizen_initialize();
            new ReactNativeApplication().Run(args);
        }
    }
}
