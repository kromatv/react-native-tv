using System;
using System.IO;
using System.Net.Http;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using Tizen.Applications;

namespace ReactNativeTizen
{
    internal sealed class PreviewRefresher : ServiceApplication
    {
        private const int MaxPreviewBytes = 1024 * 1024;
        private static readonly TimeSpan FetchTimeout = TimeSpan.FromSeconds(10);

        protected override async void OnAppControlReceived(AppControlReceivedEventArgs e)
        {
            base.OnAppControlReceived(e);
            var control = e.ReceivedAppControl;
            Log("info", "Smart Hub preview service: started by " + (control.CallerApplicationId ?? "the system") + " with " + control.Operation);
            try
            {
                await Refresh();
            }
            catch (Exception error)
            {
                Log("warn", "Smart Hub preview service: " + error.GetType().Name + ": " + error.Message);
            }
            Exit();
        }

        private async Task Refresh()
        {
            var source = PreviewSource.Read(DirectoryInfo.Data);
            if (source == null)
            {
                Log("info", "Smart Hub preview service: no source set, nothing to fetch");
                return;
            }
            var refreshMinutes = RefreshMinutes(DirectoryInfo.Resource);
            if (!source.IsDue(DirectoryInfo.Data, refreshMinutes))
            {
                Log("info", "Smart Hub preview service: fetched less than " + refreshMinutes + " minutes ago");
                return;
            }
            var json = await Fetch(source);
            source.MarkFetched(DirectoryInfo.Data);
            File.WriteAllText(Path.Combine(DirectoryInfo.Data, "preview.json"), json);
            var channel = PreviewPublisher.Publish(json, ApplicationInfo.PackageId, message => Log("info", message));
            Log("info", "Smart Hub preview service: " + (channel == PreviewPublisher.NotPublished ? "no channel took the preview" : "published through channel " + channel));
        }

        private static async Task<string> Fetch(PreviewSource source)
        {
            using var client = new HttpClient { Timeout = FetchTimeout, MaxResponseContentBufferSize = MaxPreviewBytes };
            using var request = new HttpRequestMessage(HttpMethod.Get, source.Url);
            foreach (var header in source.Headers) request.Headers.TryAddWithoutValidation(header.Key, header.Value);
            using var response = await client.SendAsync(request, CancellationToken.None);
            response.EnsureSuccessStatusCode();
            var json = await response.Content.ReadAsStringAsync();
            using var preview = JsonDocument.Parse(json);
            if (!preview.RootElement.TryGetProperty("sections", out var sections) || sections.ValueKind != JsonValueKind.Array)
            {
                throw new InvalidDataException(source.Url + " returned JSON without a sections array");
            }
            return json;
        }

        private static int RefreshMinutes(string resourceDir)
        {
            using var config = JsonDocument.Parse(File.ReadAllText(Path.Combine(resourceDir, "react-native-tizen.json")));
            return config.RootElement.TryGetProperty("preview", out var preview) &&
                preview.TryGetProperty("refreshMinutes", out var minutes) && minutes.TryGetInt32(out var value)
                ? value
                : 10;
        }

        private void Log(string level, string message) => DevLog.Write(DirectoryInfo.Resource, level, message);

        private static void Main(string[] args)
        {
            new PreviewRefresher().Run(args);
        }
    }
}
