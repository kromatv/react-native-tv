#include <react-native-tizen/react-native-tizen.h>

#include "ReactHost.h"
#include "intl/IcuData.h"
#include "logging/Log.h"
#include "threading/MainThread.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <exception>
#include <functional>
#include <memory>
#include <string>

struct rn_tizen_host {
  std::unique_ptr<rntizen::ReactHost> host;
};

namespace {

char *copyString(const std::string &text) {
  auto copy = std::make_unique_for_overwrite<char[]>(text.size() + 1);
  std::ranges::copy(text, copy.get());
  copy[text.size()] = '\0';
  return copy.release();
}

template <typename Result>
Result onMainThread(Result fallback, const std::function<Result()> &body) {
  Result result = fallback;
  rntizen::runOnMainThreadSync([&result, &body] { result = body(); });
  return result;
}

} // namespace

extern "C" {

// Evas reads `font_size` as points at 75 DPI unless told otherwise, which draws
// every glyph 4% larger than React Native's pixels (docs/reference/efl-rendering.md).
void rn_tizen_initialize(void) {
  setenv("EVAS_FONT_DPI", "72", 0);
}

rn_tizen_host *rn_tizen_host_create(const char *resource_dir) {
  if (resource_dir == nullptr) {
    rntizen::log(rntizen::LogLevel::Error, "rn_tizen_host_create: resource_dir is NULL");
    return nullptr;
  }
  rntizen::useIcuData(resource_dir);
  try {
    return std::make_unique<rn_tizen_host>(std::make_unique<rntizen::ReactHost>(resource_dir)).release();
  } catch (const std::exception &error) {
    rntizen::log(rntizen::LogLevel::Error, std::string("React Native failed to start: ") + error.what());
    return nullptr;
  }
}

void rn_tizen_host_pause(rn_tizen_host *host) {
  if (host != nullptr) host->host->pause();
}

void rn_tizen_host_resume(rn_tizen_host *host) {
  if (host != nullptr) host->host->resume();
}

void rn_tizen_host_set_initial_url(rn_tizen_host *host, const char *url) {
  if (host != nullptr && url != nullptr) host->host->setInitialUrl(url);
}

void rn_tizen_host_open_url(rn_tizen_host *host, const char *url) {
  if (host != nullptr && url != nullptr) host->host->openUrl(url);
}

void rn_tizen_host_set_launched_from_tile(rn_tizen_host *host, int from_tile) {
  if (host != nullptr) host->host->tv().setLaunchedFromTile(from_tile != 0);
}

void rn_tizen_host_set_voice_available(rn_tizen_host *host, int available) {
  if (host != nullptr) host->host->tv().setVoiceAvailable(available != 0);
}

char *rn_tizen_host_voice_state(rn_tizen_host *host) {
  if (host == nullptr) return copyString("None");
  return copyString(onMainThread<std::string>("None", [host] { return host->host->voice().appState(); }));
}

char *rn_tizen_host_voice_context(rn_tizen_host *host) {
  if (host == nullptr) return copyString("[]");
  return copyString(onMainThread<std::string>("[]", [host] { return host->host->voice().contentContext(); }));
}

int rn_tizen_host_voice_command(rn_tizen_host *host, const char *json) {
  if (host == nullptr || json == nullptr) return 0;
  return onMainThread<int>(0, [host, json] { return host->host->voice().handle(json) ? 1 : 0; });
}

void rn_tizen_host_set_preview_publisher(rn_tizen_host *host, rn_tizen_preview_publisher publisher) {
  if (host == nullptr) return;
  if (publisher == nullptr) {
    host->host->tv().setPreviewPublisher(nullptr);
    return;
  }
  host->host->tv().setPreviewPublisher([publisher](const std::string &json) { return publisher(json.c_str()); });
}

void rn_tizen_log(int level, const char *message) {
  using enum rntizen::LogLevel;
  if (message == nullptr) return;
  const auto levels = std::array{Debug, Info, Warning, Error};
  rntizen::log(levels[std::clamp(level, 0, 3)], message);
}

void rn_tizen_string_free(char *text) {
  const std::unique_ptr<char[]> owned(text);
}

void rn_tizen_host_destroy(rn_tizen_host *host) {
  const std::unique_ptr<rn_tizen_host> owned(host);
}

} // extern "C"
