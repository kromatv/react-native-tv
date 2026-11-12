#include "ProcessMemory.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace rntizen {

namespace {

struct ProcDirectory {
  std::filesystem::path path = std::filesystem::temp_directory_path() / "rn-tizen-proc-test";

  ProcDirectory() {
    std::filesystem::remove_all(path);
    std::filesystem::create_directories(path);
  }
  ~ProcDirectory() { std::filesystem::remove_all(path); }

  void write(const std::string &name, const std::string &text) const { std::ofstream(path / name) << text; }
};

} // namespace

TEST_CASE("kilobytesOf reads a /proc size line by its exact key") {
  const std::string text = "Rss:              20480 kB\nPss:              10240 kB\nPss_Anon:  1 kB\nSwapPss: 512 kB\n";

  CHECK(kilobytesOf(text, "Pss") == 10240);
  CHECK(kilobytesOf(text, "SwapPss") == 512);
  CHECK_FALSE(kilobytesOf(text, "Swap").has_value());
  CHECK_FALSE(kilobytesOf("Pss:\n", "Pss").has_value());
  CHECK_FALSE(kilobytesOf("Pss: many kB\n", "Pss").has_value());
}

TEST_CASE_FIXTURE(ProcDirectory, "readProcessMemory takes PSS and swap from smaps_rollup") {
  write("smaps_rollup", "00010000-ffff0000 ---p 00000000 00:00 0  [rollup]\nRss: 4096 kB\nPss: 2048 kB\nSwapPss: 1024 kB\n");
  write("status", "VmRSS: 4096 kB\nVmSwap: 0 kB\n");

  const auto memory = readProcessMemory(path.string());

  CHECK(memory.pssMb == 2);
  CHECK(memory.swapMb == 1);
  CHECK(memory.source == "smaps_rollup");
}

TEST_CASE_FIXTURE(ProcDirectory, "readProcessMemory falls back to RSS without smaps_rollup, and reads nothing without either") {
  write("status", "Name: app\nVmRSS: 3072 kB\nVmSwap: 512 kB\n");

  const auto memory = readProcessMemory(path.string());
  std::filesystem::remove(path / "status");
  const auto none = readProcessMemory(path.string());

  CHECK(memory.pssMb == 3);
  CHECK(memory.swapMb == 0.5);
  CHECK(memory.source == "status");
  CHECK(none.source.empty());
}

} // namespace rntizen
