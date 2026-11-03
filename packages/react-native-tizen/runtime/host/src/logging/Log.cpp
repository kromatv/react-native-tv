#include "Log.h"

#include <dlog.h>

#include <mutex>
#include <string>

namespace rntizen {

namespace {

struct InstalledSink {
  std::mutex mutex;
  std::shared_ptr<LogSink> sink;
};

InstalledSink &installedSink() {
  static InstalledSink installed;
  return installed;
}

std::shared_ptr<LogSink> currentSink() {
  auto &installed = installedSink();
  std::lock_guard lock(installed.mutex);
  return installed.sink;
}

log_priority dlogPriority(LogLevel level) {
  switch (level) {
    case LogLevel::Debug:
      return DLOG_DEBUG;
    case LogLevel::Info:
      return DLOG_INFO;
    case LogLevel::Warning:
      return DLOG_WARN;
    case LogLevel::Error:
      return DLOG_ERROR;
  }
  return DLOG_INFO;
}

} // namespace

std::string_view logLevelName(LogLevel level) {
  switch (level) {
    case LogLevel::Debug:
      return "debug";
    case LogLevel::Info:
      return "info";
    case LogLevel::Warning:
      return "warn";
    case LogLevel::Error:
      return "error";
  }
  return "info";
}

void log(LogLevel level, std::string_view message) {
  const std::string line(message);
  dlog_print(dlogPriority(level), "ReactNative", "%s", line.c_str());
  if (auto sink = currentSink()) {
    sink->write(level, message);
  }
}

void setLogSink(std::shared_ptr<LogSink> sink) {
  auto &installed = installedSink();
  std::lock_guard lock(installed.mutex);
  installed.sink = std::move(sink);
}

} // namespace rntizen
