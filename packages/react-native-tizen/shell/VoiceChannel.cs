using System;
using System.Reflection;
using Tizen.Applications;
using Tizen.Applications.EventManager;
using Tizen.Applications.Messages;

namespace ReactNativeTizen
{
    internal sealed class VoiceChannel : IDisposable
    {
        private const string VoiceApp = "com.samsung.tv.vif";
        private const string PortName = "com.port.vif.voiceinteraction";
        private const string InitEventName = "event.com.samsung.tv.vif.InitEvent";
        private const string ProtocolVersion = "1.0.8";
        private const string SamsungHandler =
            "Tizen.TV.Service.VoiceInteraction.VoiceInteractionHandler, Tizen.TV.Service.VoiceInteraction";

        private readonly IntPtr host;
        private readonly string appId;
        private readonly int mainThread = Environment.CurrentManagedThreadId;
        private bool threadLogged;
        private MessagePort port;
        private EventReceiver initEvents;

        private VoiceChannel(IntPtr host, string appId)
        {
            this.host = host;
            this.appId = appId;
        }

        internal static VoiceChannel Start(IntPtr host)
        {
            ProbeSamsungHandler();
            var channel = new VoiceChannel(host, Application.Current.ApplicationInfo.ApplicationId);
            try
            {
                channel.Open();
                channel.initEvents = new EventReceiver(InitEventName);
                channel.initEvents.Received += channel.OnVoiceServiceStarted;
                return channel;
            }
            catch (Exception error)
            {
                NativeHost.Warn("Voice: cannot reach Samsung's voice service: " + error.Message);
                channel.Dispose();
                return null;
            }
        }

        private static void ProbeSamsungHandler()
        {
            var type = Type.GetType(SamsungHandler, false);
            if (type == null)
            {
                NativeHost.Info("Voice: Tizen.TV.Service.VoiceInteraction is not on this TV");
                return;
            }
            var version = type.Assembly.GetName().Version;
            var checker = type.Assembly.GetType("Tizen.TV.Service.VoiceInteraction.PrivilegeChecker");
            var check = checker?.GetMethod("CheckVoiceInteractionPrivilege", BindingFlags.Static | BindingFlags.Public);
            try
            {
                check?.Invoke(null, null);
                NativeHost.Info("Voice: Tizen.TV.Service.VoiceInteraction " + version + " accepts this app");
            }
            catch (TargetInvocationException refusal)
            {
                NativeHost.Info("Voice: Tizen.TV.Service.VoiceInteraction " + version + " refuses this app: " +
                    refusal.InnerException?.GetType().Name + " " + refusal.InnerException?.Message);
            }
        }

        private void Open()
        {
            if (port == null)
            {
                port = new MessagePort(PortName, false);
                port.MessageReceived += OnMessage;
            }
            if (!port.Listening) port.Listen();
            Send("register", appId);
            NativeHost.Info("Voice: registered " + appId + " with " + VoiceApp);
        }

        private void OnVoiceServiceStarted(object sender, EventManagerEventArgs e)
        {
            NativeHost.Info("Voice: the voice service restarted, registering again");
            try
            {
                Open();
            }
            catch (Exception error)
            {
                NativeHost.Warn("Voice: cannot register again: " + error.Message);
            }
        }

        private void OnMessage(object sender, MessageReceivedEventArgs e)
        {
            if (!threadLogged)
            {
                threadLogged = true;
                var thread = Environment.CurrentManagedThreadId == mainThread ? "the main thread" : "a worker thread";
                NativeHost.Info("Voice: messages from " + e.Remote?.AppId + " arrive on " + thread);
            }
            if (e.Remote?.AppId != VoiceApp || e.Message == null) return;
            string command = string.Empty;
            string data = string.Empty;
            e.Message.TryGetItem("command", out command);
            e.Message.TryGetItem("strData", out data);
            try
            {
                Answer(command ?? string.Empty, data ?? string.Empty);
            }
            catch (Exception error)
            {
                NativeHost.Warn("Voice: " + command + " failed: " + error.Message);
            }
        }

        private void Answer(string command, string data)
        {
            switch (command)
            {
                case "ACTION_REQUEST_APP_CONTEXT":
                    Send("appContext", NativeHost.TakeString(NativeHost.rn_tizen_host_voice_state(host)));
                    return;
                case "ACTION_REQUEST_CONTENT_CONTEXT":
                    Send("result", NativeHost.TakeString(NativeHost.rn_tizen_host_voice_context(host)));
                    return;
                case "ACTION_SEARCH_COLLECTION":
                    Send("result", "[]");
                    return;
                case "ACTION_SET_LAYOUT_UPDATED_LISTENER":
                    Send("result", "success");
                    return;
            }
            var json = VoiceCommands.ToJson(command, data);
            var handled = json != null && NativeHost.rn_tizen_host_voice_command(host, json) != 0;
            NativeHost.Info("Voice: " + command + (handled ? " handled" : " left to the TV"));
            if (data.Length > 0) NativeHost.Debug("Voice: " + command + " data: " + data);
            Send("result", handled ? "success" : "not_support");
        }

        private void Send(string command, string data)
        {
            using var message = new Bundle();
            message.AddItem("command", command);
            message.AddItem("strData", data);
            message.AddItem("totalDataLength", data.Length.ToString());
            message.AddItem("sdkVersion", ProtocolVersion);
            message.AddItem("description", string.Empty);
            port?.Send(message, VoiceApp, PortName);
        }

        public void Dispose()
        {
            if (initEvents != null)
            {
                initEvents.Received -= OnVoiceServiceStarted;
                initEvents.Dispose();
                initEvents = null;
            }
            if (port == null) return;
            try
            {
                Send("unregister", appId);
                port.StopListening();
            }
            catch (Exception)
            {
            }
            port.MessageReceived -= OnMessage;
            port.Dispose();
            port = null;
        }
    }
}
