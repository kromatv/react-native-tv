using System;
using System.IO;
using System.Text.Json;

namespace ReactNativeTizen
{
    internal sealed class ShellConfig
    {
        internal bool Voice { get; private set; }
        internal bool PersonalPreview { get; private set; }
        internal string PreviewService { get; private set; }

        internal static ShellConfig Read(string resourceDir)
        {
            var config = new ShellConfig();
            try
            {
                using var json = JsonDocument.Parse(File.ReadAllText(Path.Combine(resourceDir, "react-native-tizen.json")));
                var root = json.RootElement;
                config.Voice = root.TryGetProperty("voice", out var voice) && voice.ValueKind == JsonValueKind.Object;
                if (root.TryGetProperty("preview", out var preview) && preview.ValueKind == JsonValueKind.Object)
                {
                    config.PersonalPreview = preview.TryGetProperty("kind", out var kind) && kind.GetString() == "personal";
                    if (preview.TryGetProperty("service", out var service) && service.ValueKind == JsonValueKind.String)
                    {
                        config.PreviewService = service.GetString();
                    }
                }
            }
            catch (Exception error)
            {
                Tizen.Log.Error("ReactNative", "Cannot read react-native-tizen.json: " + error.Message);
            }
            return config;
        }
    }
}
