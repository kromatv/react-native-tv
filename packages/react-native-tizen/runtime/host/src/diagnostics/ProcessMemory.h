#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace rntizen {

/// The app process's memory as the kernel accounts it, in MB: what Samsung's
/// budget counts that a process can read about itself (PSS and swap; GPU and
/// GEM memory are not in `/proc`).
struct ProcessMemory {
  double pssMb{0};
  double swapMb{0};
  /// `smaps_rollup` when PSS and SwapPss were read; `status` when the kernel
  /// has no `smaps_rollup` and RSS and VmSwap stand in, counting shared pages in full.
  std::string source;
};

/// The size in kB a `/proc` line `<key>:  <n> kB` gives, the first one in `text`.
std::optional<double> kilobytesOf(std::string_view text, std::string_view key);

/// Reads `<procSelf>/smaps_rollup`, else `<procSelf>/status`. An empty `source`
/// means neither could be read.
ProcessMemory readProcessMemory(const std::string &procSelf = "/proc/self");

} // namespace rntizen
