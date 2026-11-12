using System;
using System.IO;
using System.Net.Sockets;
using System.Text;
using System.Text.RegularExpressions;

namespace ReactNativeTizen
{
    internal static class DevLog
    {
        private static readonly Regex LogEndpoint = new Regex(
            "\"log\"\\s*:\\s*\\{[^}]*\"host\"\\s*:\\s*\"(?<host>[^\"]+)\"[^}]*\"port\"\\s*:\\s*(?<port>\\d+)");

        private static readonly object Gate = new object();
        private static TcpClient client;

        internal static void Write(string resourceDir, string level, string message)
        {
            Tizen.Log.Info("ReactNative", message);
            lock (Gate)
            {
                try
                {
                    client ??= Connect(resourceDir);
                    if (client == null) return;
                    var line = "{\"level\":\"" + level + "\",\"message\":" + Quote(message) + "}\n";
                    var bytes = Encoding.UTF8.GetBytes(line);
                    client.GetStream().Write(bytes, 0, bytes.Length);
                }
                catch (Exception)
                {
                    client?.Dispose();
                    client = null;
                }
            }
        }

        private static TcpClient Connect(string resourceDir)
        {
            var config = File.ReadAllText(Path.Combine(resourceDir, "react-native-tizen.json"));
            var endpoint = LogEndpoint.Match(config);
            if (!endpoint.Success) return null;
            return new TcpClient(endpoint.Groups["host"].Value, int.Parse(endpoint.Groups["port"].Value));
        }

        private static string Quote(string text)
        {
            var quoted = new StringBuilder("\"");
            foreach (var character in text)
            {
                switch (character)
                {
                    case '"': quoted.Append("\\\""); break;
                    case '\\': quoted.Append("\\\\"); break;
                    case '\n': quoted.Append("\\n"); break;
                    case '\r': break;
                    default:
                        if (character < ' ') quoted.Append("\\u").Append(((int)character).ToString("x4"));
                        else quoted.Append(character);
                        break;
                }
            }
            return quoted.Append('"').ToString();
        }
    }
}
