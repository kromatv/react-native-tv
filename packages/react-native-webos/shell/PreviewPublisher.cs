using System;
using System.Reflection;
using System.IO;
using System.Text;
using System.Text.Json;
using Tizen.Applications;
using Tizen.Applications.Messages;

namespace ReactNativeTizen
{
    internal static class PreviewPublisher
    {
        internal const int NotPublished = 0;
        internal const int ContentDb = 1;
        internal const int MessagePortChannel = 2;

        private const string ContentServiceAssembly = "Tizen.TV.Service.ContentService";
        private const string ContentServiceFile = "/usr/share/dotnet.tizen/framework/Tizen.TV.Service.ContentService.dll";
        private const string ContentServiceApp = "com.samsung.tv.contentservicemanager";
        private const string PartnerPort = "PARTNER_REQUEST_PORT";
        private const string LocalPort = "EDEN_WEBAPIS_PORT";

        private static MessagePort replies;

        internal static int Publish(string samsungJson, string packageId, Action<string> log)
        {
            var json = WithPackageName(samsungJson, packageId);
            if (TryContentDb(json, packageId, log)) return ContentDb;
            if (TryMessagePort(json, log)) return MessagePortChannel;
            return NotPublished;
        }

        private static string WithPackageName(string samsungJson, string packageId)
        {
            using var preview = JsonDocument.Parse(samsungJson);
            using var buffer = new MemoryStream();
            using (var writer = new Utf8JsonWriter(buffer))
            {
                writer.WriteStartObject();
                foreach (var property in preview.RootElement.EnumerateObject())
                {
                    if (property.Name != "package_name") property.WriteTo(writer);
                }
                writer.WriteString("package_name", packageId);
                writer.WriteEndObject();
            }
            return Encoding.UTF8.GetString(buffer.ToArray());
        }

        private static Type ContentDbApi()
        {
            Assembly assembly;
            try
            {
                assembly = Assembly.Load(new AssemblyName(ContentServiceAssembly));
            }
            catch (Exception)
            {
                assembly = System.IO.File.Exists(ContentServiceFile) ? Assembly.LoadFrom(ContentServiceFile) : null;
            }
            return assembly?.GetType("Tizen.TV.Service.ContentService.ContentDBAPI");
        }

        private static bool TryContentDb(string json, string packageId, Action<string> log)
        {
            try
            {
                var api = ContentDbApi();
                if (api == null)
                {
                    log("Smart Hub preview: ContentDBAPI is not on this TV");
                    return false;
                }
                var setPreview = api.GetMethod("SetPreviewMetadata", new[] { typeof(string), typeof(string) });
                var result = setPreview?.Invoke(null, new object[] { json, packageId });
                log("Smart Hub preview: ContentDBAPI.SetPreviewMetadata " + api.Assembly.GetName().Version + " returned " + (result ?? "nothing"));
                return result is bool accepted && accepted;
            }
            catch (TargetInvocationException refusal)
            {
                log("Smart Hub preview: ContentDBAPI refused: " + Describe(refusal.InnerException));
                return false;
            }
            catch (Exception error)
            {
                log("Smart Hub preview: ContentDBAPI failed: " + Describe(error));
                return false;
            }
        }

        private static bool TryMessagePort(string json, Action<string> log)
        {
            try
            {
                if (replies == null)
                {
                    replies = new MessagePort(LocalPort, false);
                    replies.MessageReceived += (sender, e) =>
                    {
                        string command = string.Empty;
                        e.Message?.TryGetItem("command", out command);
                        log("Smart Hub preview: " + e.Remote?.AppId + " answered " + (command ?? "?") + " " + (e.Message?.Encode() ?? ""));
                    };
                    replies.Listen();
                }
                using var message = new Bundle();
                message.AddItem("DATA_COMMAND", "SET_PREVIEW_DATA");
                message.AddItem("DATA", json);
                replies.Send(message, ContentServiceApp, PartnerPort);
                log("Smart Hub preview: sent SET_PREVIEW_DATA to " + ContentServiceApp + "/" + PartnerPort);
                return true;
            }
            catch (Exception error)
            {
                log("Smart Hub preview: message port failed: " + Describe(error));
            }
            return false;
        }

        private static string Describe(Exception error) =>
            error == null ? "unknown error" : error.GetType().Name + ": " + error.Message + (error.InnerException != null ? " (" + Describe(error.InnerException) + ")" : "");
    }
}
