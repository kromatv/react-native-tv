#include <app.h>

#include <Ecore.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int makeDirectories(char *path) {
  for (char *slash = strchr(path + 1, '/'); slash != NULL; slash = strchr(slash + 1, '/')) {
    *slash = '\0';
    int failed = mkdir(path, 0700) != 0 && errno != EEXIST;
    *slash = '/';
    if (failed) return -1;
  }
  return mkdir(path, 0700) != 0 && errno != EEXIST ? -1 : 0;
}

static char *withTrailingSlash(char *path, int length) {
  if (path[length - 1] != '/') strcat(path, "/");
  return strdup(path);
}

static char *appDirectory(const char *variable, const char *name) {
  char path[PATH_MAX];
  const char *configured = getenv(variable);
  const char *temporary = getenv("TMPDIR");
  int length = configured != NULL && configured[0] != '\0'
                   ? snprintf(path, sizeof path, "%s", configured)
                   : snprintf(path, sizeof path, "%s/rn-tizen-simulator/%s", temporary != NULL ? temporary : "/tmp", name);
  if (length <= 0 || (size_t)length >= sizeof path - 1) return NULL;
  if (makeDirectories(path) != 0) return NULL;
  return withTrailingSlash(path, length);
}

char *app_get_data_path(void) {
  return appDirectory("RN_TIZEN_SIMULATOR_DATA_DIR", "data");
}

char *app_get_cache_path(void) {
  return appDirectory("RN_TIZEN_SIMULATOR_CACHE_DIR", "cache");
}

char *app_get_resource_path(void) {
  char path[PATH_MAX];
  const char *configured = getenv("RN_TIZEN_SIMULATOR_RESOURCE_DIR");
  struct stat info;
  if (configured == NULL || stat(configured, &info) != 0 || !S_ISDIR(info.st_mode)) return NULL;
  int length = snprintf(path, sizeof path, "%s", configured);
  if (length <= 0 || (size_t)length >= sizeof path - 1) return NULL;
  return withTrailingSlash(path, length);
}

static void quitMainLoop(void *data) {
  (void)data;
  ecore_main_loop_quit();
}

void ui_app_exit(void) {
  ecore_main_loop_thread_safe_call_async(quitMainLoop, NULL);
}
