#include <system_info.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  const char *key;
  const char *value;
} StringFeature;

typedef struct {
  const char *key;
  int value;
} IntFeature;

static const StringFeature kStrings[] = {
    {"http://tizen.org/system/model_name", "Simulator"},
    {"http://tizen.org/system/manufacturer", "react-native-tizen"},
    {"http://tizen.org/system/platform.name", "Tizen"},
    {"http://tizen.org/feature/platform.version", "9.0"},
    {"http://tizen.org/feature/profile", "tv"},
    {"http://tizen.org/system/build.string", "rn-tizen-simulator"},
};

static const IntFeature kInts[] = {
    {"http://tizen.org/feature/screen.width", 1920},
    {"http://tizen.org/feature/screen.height", 1080},
    {"http://tizen.org/feature/screen.bpp", 32},
};

static int screenSize(const char *key, int *value) {
  const char *configured = getenv("RN_TIZEN_SIMULATOR_SCREEN_SIZE");
  int width = 0;
  int height = 0;
  if (configured == NULL || sscanf(configured, "%dx%d", &width, &height) != 2 || width <= 0 || height <= 0) return 0;
  if (strcmp(key, "http://tizen.org/feature/screen.width") == 0) {
    *value = width;
    return 1;
  }
  if (strcmp(key, "http://tizen.org/feature/screen.height") == 0) {
    *value = height;
    return 1;
  }
  return 0;
}

int system_info_get_platform_int(const char *key, int *value) {
  if (key == NULL || value == NULL) return SYSTEM_INFO_ERROR_INVALID_PARAMETER;
  if (screenSize(key, value)) return SYSTEM_INFO_ERROR_NONE;
  for (size_t i = 0; i < sizeof kInts / sizeof kInts[0]; ++i) {
    if (strcmp(kInts[i].key, key) == 0) {
      *value = kInts[i].value;
      return SYSTEM_INFO_ERROR_NONE;
    }
  }
  return SYSTEM_INFO_ERROR_INVALID_PARAMETER;
}

int system_info_get_platform_string(const char *key, char **value) {
  if (key == NULL || value == NULL) return SYSTEM_INFO_ERROR_INVALID_PARAMETER;
  for (size_t i = 0; i < sizeof kStrings / sizeof kStrings[0]; ++i) {
    if (strcmp(kStrings[i].key, key) == 0) {
      *value = strdup(kStrings[i].value);
      return *value != NULL ? SYSTEM_INFO_ERROR_NONE : SYSTEM_INFO_ERROR_OUT_OF_MEMORY;
    }
  }
  return SYSTEM_INFO_ERROR_INVALID_PARAMETER;
}
