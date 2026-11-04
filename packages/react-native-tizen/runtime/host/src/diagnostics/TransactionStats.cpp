#include "TransactionStats.h"

#include <react/utils/Telemetry.h>

#include <algorithm>
#include <atomic>
#include <mutex>
#include <utility>

namespace rntizen {

namespace {

using facebook::react::kTelemetryUndefinedTimePoint;
using facebook::react::TelemetryTimePoint;

struct Recorder {
  std::atomic<bool> enabled{false};
  std::mutex mutex;
  TransactionStats stats;
};

Recorder &recorder() {
  static Recorder shared;
  return shared;
}

double millisBetween(TelemetryTimePoint start, TelemetryTimePoint end) {
  if (start == kTelemetryUndefinedTimePoint || end == kTelemetryUndefinedTimePoint || end < start) return 0;
  return std::chrono::duration<double, std::milli>(end - start).count();
}

} // namespace

bool transactionStatsEnabled() {
  return recorder().enabled.load();
}

void setTransactionStatsEnabled(bool on) {
  recorder().enabled.store(on);
}

void recordTransaction(
    const facebook::react::TransactionTelemetry &telemetry,
    int mutations,
    std::chrono::steady_clock::time_point mountStart,
    std::chrono::steady_clock::time_point mountEnd,
    const ComponentMountCosts &components) {
  if (!transactionStatsEnabled()) return;
  const double commit = millisBetween(telemetry.getCommitStartTime(), telemetry.getCommitEndTime());
  const double layout = millisBetween(telemetry.getLayoutStartTime(), telemetry.getLayoutEndTime());
  const double mount = millisBetween(mountStart, mountEnd);
  const double latency = millisBetween(telemetry.getCommitEndTime(), mountStart);
  auto &shared = recorder();
  std::lock_guard lock(shared.mutex);
  auto &stats = shared.stats;
  stats.count += 1;
  stats.mutations += mutations;
  stats.commitTotal += commit;
  stats.commitMax = std::max(stats.commitMax, commit);
  stats.layoutMax = std::max(stats.layoutMax, layout);
  stats.mountTotal += mount;
  stats.mountMax = std::max(stats.mountMax, mount);
  stats.latencyMax = std::max(stats.latencyMax, latency);
  for (const auto &[name, cost] : components) {
    auto &total = stats.components[name];
    total.mutations += cost.mutations;
    total.ms += cost.ms;
  }
}

TransactionStats takeTransactionStats() {
  auto &shared = recorder();
  std::lock_guard lock(shared.mutex);
  return std::exchange(shared.stats, TransactionStats{});
}

} // namespace rntizen
