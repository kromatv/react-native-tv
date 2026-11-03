#include <react-native-tizen/react-native-tizen.h>

#include <Ecore.h>
#include <Elementary.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CONTROL_LINE_MAX 4096

typedef struct {
  const char *resourceDir;
  rn_tizen_host *host;
  int exitCode;
  char pending[CONTROL_LINE_MAX];
  size_t pendingLength;
} Launcher;

static void startHost(void *data) {
  Launcher *launcher = data;
  launcher->host = rn_tizen_host_create(launcher->resourceDir);
  if (launcher->host == NULL) {
    launcher->exitCode = 1;
    ecore_main_loop_quit();
  }
}

static Eina_Bool onExitSignal(void *data, int type, void *event) {
  (void)data;
  (void)type;
  (void)event;
  ecore_main_loop_quit();
  return ECORE_CALLBACK_DONE;
}

static void runVoiceCommand(rn_tizen_host *host, const char *json) {
  char *state = rn_tizen_host_voice_state(host);
  char *context = rn_tizen_host_voice_context(host);
  int handled = rn_tizen_host_voice_command(host, json);
  size_t length = strlen(state) + strlen(context) + strlen(json) + 96;
  char *line = malloc(length);
  if (line != NULL) {
    snprintf(line, length, "Voice (simulator stub): app state %s, content context %s, %s %s", state, context, json,
             handled ? "handled" : "left to the TV");
    rn_tizen_log(1, line);
    free(line);
  }
  rn_tizen_string_free(state);
  rn_tizen_string_free(context);
}

static void runControlCommand(Launcher *launcher, const char *line) {
  static const char kOpenUrl[] = "open-url ";
  static const char kTile[] = "tile ";
  static const char kVoice[] = "voice ";
  if (strcmp(line, "exit") == 0) {
    ecore_main_loop_quit();
  } else if (launcher->host == NULL) {
    fprintf(stderr, "rn-tizen-simulator: the app is not started, ignoring \"%s\"\n", line);
  } else if (strcmp(line, "pause") == 0) {
    rn_tizen_host_pause(launcher->host);
  } else if (strcmp(line, "resume") == 0) {
    rn_tizen_host_resume(launcher->host);
  } else if (strncmp(line, kOpenUrl, sizeof kOpenUrl - 1) == 0) {
    rn_tizen_host_set_launched_from_tile(launcher->host, 0);
    rn_tizen_host_open_url(launcher->host, line + sizeof kOpenUrl - 1);
  } else if (strncmp(line, kTile, sizeof kTile - 1) == 0) {
    rn_tizen_host_set_launched_from_tile(launcher->host, 1);
    rn_tizen_host_open_url(launcher->host, line + sizeof kTile - 1);
  } else if (strncmp(line, kVoice, sizeof kVoice - 1) == 0) {
    runVoiceCommand(launcher->host, line + sizeof kVoice - 1);
  } else {
    fprintf(stderr, "rn-tizen-simulator: unknown control command \"%s\"\n", line);
  }
}

static Eina_Bool onControlReadable(void *data, Ecore_Fd_Handler *handler) {
  Launcher *launcher = data;
  int fd = ecore_main_fd_handler_fd_get(handler);
  char *buffer = launcher->pending + launcher->pendingLength;
  ssize_t count = read(fd, buffer, sizeof launcher->pending - launcher->pendingLength - 1);
  if (count <= 0) return ECORE_CALLBACK_RENEW;
  launcher->pendingLength += (size_t)count;
  launcher->pending[launcher->pendingLength] = '\0';
  char *line = launcher->pending;
  char *end = strchr(line, '\n');
  while (end != NULL) {
    *end = '\0';
    if (*line != '\0') runControlCommand(launcher, line);
    line = end + 1;
    end = strchr(line, '\n');
  }
  launcher->pendingLength = strlen(line);
  if (launcher->pendingLength == sizeof launcher->pending - 1) launcher->pendingLength = 0;
  memmove(launcher->pending, line, launcher->pendingLength);
  return ECORE_CALLBACK_RENEW;
}

static void listenForControl(Launcher *launcher, const char *path) {
  int fd = open(path, O_RDWR | O_NONBLOCK | O_CLOEXEC);
  if (fd < 0) {
    perror(path);
    return;
  }
  ecore_main_fd_handler_add(fd, ECORE_FD_READ, onControlReadable, launcher, NULL, NULL);
}

int main(int argc, char **argv) {
  if (argc != 2 && argc != 3) {
    fprintf(stderr, "usage: %s <resource-dir> [<control-fifo>]\n", argv[0]);
    return 2;
  }
  Launcher launcher = {.resourceDir = argv[1]};
  rn_tizen_initialize();
  elm_init(argc, argv);
  ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, onExitSignal, NULL);
  if (argc == 3) listenForControl(&launcher, argv[2]);
  ecore_job_add(startHost, &launcher);
  ecore_main_loop_begin();
  rn_tizen_host_destroy(launcher.host);
  elm_shutdown();
  return launcher.exitCode;
}
