using System;
using System.Runtime.InteropServices;

namespace ReactNativeTizen
{
    internal static class NativeHost
    {
        private const string Library = "libreact-native-tizen.so";

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        internal delegate int PreviewPublisher([MarshalAs(UnmanagedType.LPUTF8Str)] string json);

        [DllImport(Library)]
        internal static extern void rn_tizen_initialize();

        [DllImport(Library)]
        internal static extern IntPtr rn_tizen_host_create(string resourceDir);

        [DllImport(Library)]
        internal static extern void rn_tizen_host_pause(IntPtr host);

        [DllImport(Library)]
        internal static extern void rn_tizen_host_resume(IntPtr host);

        [DllImport(Library)]
        internal static extern void rn_tizen_host_set_initial_url(IntPtr host, [MarshalAs(UnmanagedType.LPUTF8Str)] string url);

        [DllImport(Library)]
        internal static extern void rn_tizen_host_open_url(IntPtr host, [MarshalAs(UnmanagedType.LPUTF8Str)] string url);

        [DllImport(Library)]
        internal static extern void rn_tizen_host_set_launched_from_tile(IntPtr host, int fromTile);

        [DllImport(Library)]
        internal static extern void rn_tizen_host_set_voice_available(IntPtr host, int available);

        [DllImport(Library)]
        internal static extern IntPtr rn_tizen_host_voice_state(IntPtr host);

        [DllImport(Library)]
        internal static extern IntPtr rn_tizen_host_voice_context(IntPtr host);

        [DllImport(Library)]
        internal static extern int rn_tizen_host_voice_command(IntPtr host, [MarshalAs(UnmanagedType.LPUTF8Str)] string json);

        [DllImport(Library)]
        internal static extern void rn_tizen_host_set_preview_publisher(IntPtr host, PreviewPublisher publisher);

        [DllImport(Library)]
        internal static extern void rn_tizen_log(int level, [MarshalAs(UnmanagedType.LPUTF8Str)] string message);

        [DllImport(Library)]
        internal static extern void rn_tizen_string_free(IntPtr text);

        [DllImport(Library)]
        internal static extern void rn_tizen_host_destroy(IntPtr host);

        internal static string TakeString(IntPtr text)
        {
            if (text == IntPtr.Zero) return string.Empty;
            try
            {
                return Marshal.PtrToStringUTF8(text) ?? string.Empty;
            }
            finally
            {
                rn_tizen_string_free(text);
            }
        }

        internal static void Debug(string message) => rn_tizen_log(0, message);

        internal static void Info(string message) => rn_tizen_log(1, message);

        internal static void Warn(string message) => rn_tizen_log(2, message);
    }
}
