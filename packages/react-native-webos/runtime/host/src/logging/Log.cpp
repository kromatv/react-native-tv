#include "Log.h"

#include <cstdio>
#include <exception>
#include <format>
#include <mutex>
#include <string>

namespace rnwebos {

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
  const auto line = std::format("[{}] {}\n", logLevelName(level), message);
  std::fwrite(line.data(), 1, line.size(), stderr);
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

} // namespace rnwebos
