using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.Json;
using Tizen.Applications;

namespace ReactNativeTizen
{
    internal static class TilePayload
    {
        private const string Key = "PAYLOAD";

        internal static string Url(AppControl control)
        {
            var extras = control.ExtraData;
            if (!extras.GetKeys().Contains(Key)) return null;
            string payload = null;
            if (extras.IsCollection(Key))
            {
                if (extras.TryGet(Key, out IEnumerable<string> values)) payload = values.FirstOrDefault();
            }
            else
            {
                extras.TryGet(Key, out payload);
            }
            NativeHost.Info("Smart Hub tile: PAYLOAD " + payload);
            return string.IsNullOrEmpty(payload) ? null : UrlIn(payload);
        }

        internal static void LogExtras(AppControl control)
        {
            var keys = control.ExtraData.GetKeys().ToList();
            NativeHost.Info("App control " + control.Operation + (string.IsNullOrEmpty(control.Uri) ? "" : " " + control.Uri) +
                (keys.Count == 0 ? ", no extras" : ", extras: " + string.Join(", ", keys)));
        }

        private static string UrlIn(string payload)
        {
            try
            {
                using var envelope = JsonDocument.Parse(payload);
                if (!envelope.RootElement.TryGetProperty("values", out var values) || values.ValueKind != JsonValueKind.String)
                {
                    return null;
                }
                var actionData = values.GetString() ?? string.Empty;
                if (!actionData.TrimStart().StartsWith("{")) return Uri.IsWellFormedUriString(actionData, UriKind.Absolute) ? actionData : null;
                using var action = JsonDocument.Parse(actionData);
                return action.RootElement.TryGetProperty("url", out var url) && url.ValueKind == JsonValueKind.String
                    ? url.GetString()
                    : null;
            }
            catch (JsonException error)
            {
                NativeHost.Warn("Smart Hub tile: PAYLOAD is not JSON: " + error.Message);
                return null;
            }
        }
    }
}
