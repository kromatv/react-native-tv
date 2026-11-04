#pragma once

#include "FrameStats.h"
#include "ProcessMemory.h"
#include "TransactionStats.h"

#include <cstdint>
#include <string>

namespace rntizen {

/// The Hermes heap as `getHeapInfo` last reported it.
struct HeapReading {
  double usedMb{0};
  double sizeMb{0};
  std::int64_t collections{0};
};

/// One window of `PerfMonitor`'s measurements. Times are milliseconds.
struct PerfSample {
  double uptimeMs{0};
  double windowMs{0};
  FrameStats::Window frames;
  double jsBusyMs{0};
  double jsWaitMs{0};
  double jsLagMs{0};
  TransactionStats transactions;
  HeapReading heap;
  ProcessMemory memory;
  double cpuPercent{0};
};

/// The log line `rn-tizen perf` reads: `perf ` and one JSON object, its field
/// names listed in `docs/cli.md`.
std::string perfLine(const PerfSample &sample);

} // namespace rntizen
