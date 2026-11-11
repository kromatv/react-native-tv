#include "CrashReport.h"

#include "CrashReporter.h"

#include <doctest/doctest.h>

#include <signal.h>
#include <unistd.h>

#include <array>
#include <bit>
#include <string>

namespace rntizen {

TEST_CASE("CrashReport names the signal and the fault address, then a line per frame") {
  CrashReport report(SIGSEGV, 0xdead);

  report.addFrame(0, std::bit_cast<std::uintptr_t>(&installCrashReporter));
  report.addFrame(12, 0x10);
  report.close();

  const std::string text(report.text());
  CHECK(text.starts_with("\n{\"level\":\"error\",\"message\":\"Native crash: SIGSEGV at 0xdead\\n#0 "));
  CHECK(text.find("+0x") != std::string::npos);
  CHECK(text.ends_with("\\n#12 0x10\"}\n"));
}

TEST_CASE("CrashReport names each crash signal it handles") {
  CHECK(std::string(CrashReport(SIGBUS, 0).text()).ends_with("SIGBUS at 0x0"));
  CHECK(std::string(CrashReport(SIGILL, 1).text()).ends_with("SIGILL at 0x1"));
  CHECK(std::string(CrashReport(SIGFPE, 2).text()).ends_with("SIGFPE at 0x2"));
  CHECK(std::string(CrashReport(SIGABRT, 3).text()).ends_with("SIGABRT at 0x3"));
  CHECK(std::string(CrashReport(SIGTERM, 4).text()).ends_with("signal at 0x4"));
}

TEST_CASE("CrashReport stops at 8 KiB") {
  CrashReport report(SIGSEGV, 0);

  for (int index = 0; index < 1000; ++index) report.addFrame(index, 0x10);

  CHECK(report.text().size() == 8192);
}

TEST_CASE("CrashReport writes the whole report to a descriptor") {
  std::array<int, 2> ends{};
  REQUIRE(pipe(ends.data()) == 0);
  CrashReport report(SIGABRT, 0x1);
  report.close();

  report.writeTo(ends[1]);
  close(ends[1]);

  std::array<char, 256> read{};
  const auto length = ::read(ends[0], read.data(), read.size());
  close(ends[0]);
  CHECK(std::string(read.data(), static_cast<std::size_t>(length)) == std::string(report.text()));
}

} // namespace rntizen
