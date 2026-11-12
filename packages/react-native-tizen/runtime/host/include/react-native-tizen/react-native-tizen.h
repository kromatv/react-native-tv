#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define RN_TIZEN_EXPORT __attribute__((visibility("default")))

/// A running React Native app: its JavaScript runtime, its renderer and its window.
typedef struct rn_tizen_host rn_tizen_host;

/// Prepares the process before the app framework starts EFL: font sizes in
/// pixels, as React Native means them. Call it first, before the app's main loop.
RN_TIZEN_EXPORT void rn_tizen_initialize(void);

/// Starts React Native in the calling app. Call it on the main thread from the
/// app's create callback, once the EFL main loop runs. `resource_dir` is the
/// package's `res` directory, which holds `react-native-tizen.json` and the
/// bundle. Returns NULL, after logging why, when the app cannot start.
RN_TIZEN_EXPORT rn_tizen_host *rn_tizen_host_create(const char *resource_dir);

/// The app left the foreground; `AppState` turns `background`.
RN_TIZEN_EXPORT void rn_tizen_host_pause(rn_tizen_host *host);

/// The app came back; `AppState` turns `active`.
RN_TIZEN_EXPORT void rn_tizen_host_resume(rn_tizen_host *host);

/// The URI of the app control that launched the app, which `Linking.getInitialURL`
/// answers. Call it for the first app control only, and only when it has a URI.
RN_TIZEN_EXPORT void rn_tizen_host_set_initial_url(rn_tizen_host *host, const char *url);

/// The app was opened again with a deep link; React Native's `Linking` emits it.
RN_TIZEN_EXPORT void rn_tizen_host_open_url(rn_tizen_host *host, const char *url);

/// Whether the current launch or app control came from a Smart Hub preview
/// tile, which JavaScript reads to exit on Back with no confirmation. Call it
/// for every app control, before the URL calls.
RN_TIZEN_EXPORT void rn_tizen_host_set_launched_from_tile(rn_tizen_host *host, int from_tile);

/// Whether Samsung's voice service took the app's registration; JavaScript's
/// `Voice.isAvailable` answers it.
RN_TIZEN_EXPORT void rn_tizen_host_set_voice_available(rn_tizen_host *host, int available);

/// The app state Samsung's voice service asks for before each utterance:
/// `Home`, `List`, `Details`, `Player`, `Search` or `Setting`. Free it with
/// rn_tizen_string_free.
RN_TIZEN_EXPORT char *rn_tizen_host_voice_state(rn_tizen_host *host);

/// The titles on screen, as Samsung's content context JSON. Free it with rn_tizen_string_free.
RN_TIZEN_EXPORT char *rn_tizen_host_voice_context(rn_tizen_host *host);

/// Runs one voice command, the JSON the shell builds (`{"type":"play"}`...).
/// Returns 1 when the app handled it, 0 to leave it to the TV.
RN_TIZEN_EXPORT int rn_tizen_host_voice_command(rn_tizen_host *host, const char *json);

/// Publishes Samsung's Smart Hub preview JSON; returns the channel that took
/// it, 0 for none. Called on the main thread.
typedef int (*rn_tizen_preview_publisher)(const char *json);

/// Lets JavaScript's `SmartHubPreview` publish through the shell.
RN_TIZEN_EXPORT void rn_tizen_host_set_preview_publisher(rn_tizen_host *host, rn_tizen_preview_publisher publisher);

/// Writes `message` to the app's log, which a development build streams to
/// `rn-tizen`. `level` is 0 debug, 1 info, 2 warning, 3 error.
RN_TIZEN_EXPORT void rn_tizen_log(int level, const char *message);

/// Frees a string the runtime returned.
RN_TIZEN_EXPORT void rn_tizen_string_free(char *text);

/// Stops JavaScript, destroys the window and frees the host. The pointer is invalid afterwards.
RN_TIZEN_EXPORT void rn_tizen_host_destroy(rn_tizen_host *host);

#ifdef __cplusplus
}
#endif
