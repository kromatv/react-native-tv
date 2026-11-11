#include "Log.h"

#include <dlog.h>

#include <exception>
#include <format>
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
  using enum LogLevel;
  switch (level) {
    case Debug:
      return DLOG_DEBUG;
    case Info:
      return DLOG_INFO;
    case Warning:
      return DLOG_WARN;
    case Error:
      return DLOG_ERROR;
  }
  return DLOG_INFO;
}

} // namespace

std::string_view logLevelName(LogLevel level) {
  using enum LogLevel;
  switch (level) {
    case Debug:
      return "debug";
    case Info:
      return "info";
    case Warning:
      return "warn";
    case Error:
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

void logUncaughtException(std::string_view boundary, const std::exception &error) noexcept {
  log(LogLevel::Error, std::format("Uncaught exception on {}: {}", boundary, error.what()));
}

void logUncaughtException(std::string_view boundary) noexcept {
  log(LogLevel::Error, std::format("Uncaught exception on {}, not a std::exception", boundary));
}

void setLogSink(std::shared_ptr<LogSink> sink) {
  auto &installed = installedSink();
  std::lock_guard lock(installed.mutex);
  installed.sink = std::move(sink);
}

} // namespace rntizen
