#include "CrashReporter.h"

#include <doctest/doctest.h>

#include <signal.h>
#include <unistd.h>

#include <array>
#include <csignal>
#include <string>

namespace rntizen {

namespace {

volatile std::sig_atomic_t chained = 0;

void recordChained(int signal) {
  chained = signal;
}

std::string drain(int descriptor) {
  std::string text;
  std::array<char, 4096> chunk{};
  for (auto length = read(descriptor, chunk.data(), chunk.size()); length > 0;
       length = read(descriptor, chunk.data(), chunk.size())) {
    text.append(chunk.data(), static_cast<std::size_t>(length));
  }
  return text;
}

} // namespace

TEST_CASE("installCrashReporter reports a crash, then hands the signal to the handler before it") {
  struct sigaction recording{};
  recording.sa_handler = recordChained;
  sigemptyset(&recording.sa_mask);
  sigaction(SIGABRT, &recording, nullptr);
  sigaction(SIGSEGV, &recording, nullptr);
  std::array<int, 2> ends{};
  REQUIRE(pipe(ends.data()) == 0);
  setCrashReportDescriptor(ends[1]);
  installCrashReporter();

  raise(SIGABRT);
  const auto afterAbort = chained;
  raise(SIGSEGV);
  const auto afterFault = chained;
  setCrashReportDescriptor(-1);
  close(ends[1]);
  const auto reports = drain(ends[0]);
  close(ends[0]);

  CHECK(afterAbort == SIGABRT);
  CHECK(afterFault == SIGSEGV);
  CHECK(reports.find("Native crash: SIGABRT at 0x") != std::string::npos);
  CHECK(reports.find("\\n#0 ") != std::string::npos);
}

} // namespace rntizen
