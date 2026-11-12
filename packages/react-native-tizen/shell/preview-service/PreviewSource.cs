using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text.Json;

namespace ReactNativeTizen
{
    internal sealed class PreviewSource
    {
        private const string SourceFile = "preview-source.json";
        private const string FetchedFile = "preview-fetched-at";

        internal string Url { get; private set; }
        internal Dictionary<string, string> Headers { get; } = new Dictionary<string, string>();

        internal static PreviewSource Read(string dataDir)
        {
            var path = Path.Combine(dataDir, SourceFile);
            if (!File.Exists(path)) return null;
            using var json = JsonDocument.Parse(File.ReadAllText(path));
            var root = json.RootElement;
            if (!root.TryGetProperty("url", out var url) || url.ValueKind != JsonValueKind.String) return null;
            var source = new PreviewSource { Url = url.GetString() };
            if (root.TryGetProperty("headers", out var headers) && headers.ValueKind == JsonValueKind.Object)
            {
                foreach (var header in headers.EnumerateObject())
                {
                    if (header.Value.ValueKind == JsonValueKind.String) source.Headers[header.Name] = header.Value.GetString();
                }
            }
            return source;
        }

        internal bool IsDue(string dataDir, int refreshMinutes)
        {
            var path = Path.Combine(dataDir, FetchedFile);
            if (!File.Exists(path)) return true;
            var text = File.ReadAllText(path).Trim();
            if (!long.TryParse(text, NumberStyles.Integer, CultureInfo.InvariantCulture, out var seconds)) return true;
            var last = DateTimeOffset.FromUnixTimeSeconds(seconds);
            return DateTimeOffset.UtcNow - last >= TimeSpan.FromMinutes(refreshMinutes);
        }

        internal void MarkFetched(string dataDir)
        {
            File.WriteAllText(
                Path.Combine(dataDir, FetchedFile),
                DateTimeOffset.UtcNow.ToUnixTimeSeconds().ToString(CultureInfo.InvariantCulture));
        }
    }
}
