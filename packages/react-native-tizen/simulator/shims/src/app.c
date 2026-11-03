#include <app.h>

#include <Ecore.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct {
  const char *variable;
  const char *xdgVariable;
  const char *homeBase;
  const char *name;
} AppDirectory;

static const AppDirectory kData = {"RN_TIZEN_SIMULATOR_DATA_DIR", "XDG_DATA_HOME", ".local/share", "data"};
static const AppDirectory kCache = {"RN_TIZEN_SIMULATOR_CACHE_DIR", "XDG_CACHE_HOME", ".cache", "cache"};

static int makeDirectories(char *path) {
  for (size_t i = 1; path[i] != '\0'; ++i) {
    if (path[i] != '/') continue;
    path[i] = '\0';
    int failed = mkdir(path, 0700) != 0 && errno != EEXIST;
    path[i] = '/';
    if (failed) return -1;
  }
  return mkdir(path, 0700) != 0 && errno != EEXIST ? -1 : 0;
}

static char *withTrailingSlash(char *path, int length) {
  if (path[length - 1] != '/') strcat(path, "/");
  return strdup(path);
}

static int userDirectory(char *path, size_t size, const AppDirectory *directory) {
  const char *base = getenv(directory->xdgVariable);
  if (base != NULL && base[0] == '/') return snprintf(path, size, "%s/rn-tizen-simulator/%s", base, directory->name);
  const char *home = getenv("HOME");
  if (home == NULL || home[0] != '/') return -1;
  return snprintf(path, size, "%s/%s/rn-tizen-simulator/%s", home, directory->homeBase, directory->name);
}

static char *appDirectory(const AppDirectory *directory) {
  char path[PATH_MAX];
  const char *configured = getenv(directory->variable);
  int length = configured != NULL && configured[0] != '\0' ? snprintf(path, sizeof path, "%s", configured)
                                                           : userDirectory(path, sizeof path, directory);
  if (length <= 0 || (size_t)length >= sizeof path - 1) return NULL;
  if (makeDirectories(path) != 0) return NULL;
  return withTrailingSlash(path, length);
}

char *app_get_data_path(void) {
  return appDirectory(&kData);
}

char *app_get_cache_path(void) {
  return appDirectory(&kCache);
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
