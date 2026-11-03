using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text.Json;
using Tizen.Applications;

namespace ReactNativeTizen
{
    internal static class VoiceCommands
    {
        private static readonly Dictionary<string, string> SearchFields = new Dictionary<string, string>
        {
            ["utterance"] = "utterance",
            ["title"] = "title",
            ["genre"] = "genre",
            ["cast"] = "cast",
            ["content_type"] = "contentType",
            ["from"] = "releasedFrom",
            ["to"] = "releasedTo",
        };

        internal static string ToJson(string command, string data)
        {
            switch (command)
            {
                case "ACTION_PLAY": return Type("play");
                case "ACTION_PAUSE": return Type("pause");
                case "ACTION_STOP": return Type("stop");
                case "ACTION_MEDIA_RESTART": return Type("restart");
                case "ACTION_MEDIA_TRACK_NEXT": return Type("next");
                case "ACTION_MEDIA_TRACK_PREVIOUS": return Type("previous");
                case "ACTION_FAST_FORWARD": return Type("fastForward");
                case "ACTION_REWIND": return Type("rewind");
                case "ACTION_MEDIA_SKIP_FORWARD": return Number("skip", "seconds", Seconds(data));
                case "ACTION_MEDIA_SKIP_BACKWARD": return Number("skip", "seconds", -Seconds(data));
                case "ACTION_MEDIA_SET_PLAY_POSITION": return Number("seek", "position", Seconds(data));
                case "ACTION_MEDIA_SET_SUBTITLE":
                    return Write(writer =>
                    {
                        writer.WriteString("type", "subtitles");
                        writer.WriteBoolean("enabled", data.Trim() == "on");
                    });
                case "ACTION_TITLE_SELECTION":
                    return Write(writer =>
                    {
                        writer.WriteString("type", "selectTitle");
                        writer.WriteString("title", data.Trim());
                    });
                case "ACTION_SELECTION":
                    return int.TryParse(data, NumberStyles.Integer, CultureInfo.InvariantCulture, out var index)
                        ? Number("selectIndex", "index", index)
                        : null;
                case "ACTION_SEARCH": return Search(data);
                default: return null;
            }
        }

        private static int Seconds(string data)
        {
            int.TryParse(data, NumberStyles.Any, CultureInfo.InvariantCulture, out var seconds);
            return seconds;
        }

        private static string Type(string type) => Write(writer => writer.WriteString("type", type));

        private static string Number(string type, string field, int value) =>
            Write(writer =>
            {
                writer.WriteString("type", type);
                writer.WriteNumber(field, value);
            });

        private static string Search(string data)
        {
            using var terms = Bundle.Decode(data);
            return Write(writer =>
            {
                writer.WriteString("type", "search");
                foreach (var field in SearchFields)
                {
                    if (terms.TryGetItem(field.Key, out string value) && !string.IsNullOrEmpty(value))
                    {
                        writer.WriteString(field.Value, value);
                    }
                }
            });
        }

        private static string Write(Action<Utf8JsonWriter> body)
        {
            using var buffer = new System.IO.MemoryStream();
            using (var writer = new Utf8JsonWriter(buffer))
            {
                writer.WriteStartObject();
                body(writer);
                writer.WriteEndObject();
            }
            return System.Text.Encoding.UTF8.GetString(buffer.ToArray());
        }
    }
}
