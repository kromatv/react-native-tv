#pragma once

#include <react/renderer/telemetry/TransactionTelemetry.h>

#include <chrono>
#include <functional>
#include <map>
#include <string>

namespace rntizen {

/// What mounting one kind of component cost the main loop: its mutations and their time.
struct ComponentMountCost {
  int mutations{0};
  double ms{0};
};

/// What each kind of component cost, by component name.
using ComponentMountCosts = std::map<std::string, ComponentMountCost, std::less<>>;

/// What the Fabric transactions mounted in one window cost, in milliseconds.
/// Commit and layout run on the JavaScript thread; diff and mount on the main
/// loop; latency is how long a commit waited before the main loop mounted it.
struct TransactionStats {
  int count{0};
  int mutations{0};
  double commitTotal{0};
  double commitMax{0};
  double layoutMax{0};
  double mountTotal{0};
  double mountMax{0};
  double latencyMax{0};
  ComponentMountCosts components;
};

/// Whether `recordTransaction` keeps anything. Off until a development build's
/// `PerfMonitor` turns it on, so a release build records nothing.
bool transactionStatsEnabled();
void setTransactionStatsEnabled(bool enabled);

/// Adds one transaction the main loop mounted: `telemetry` as Fabric filled it
/// from commit to diff, when its mutations started and finished applying, and
/// what they cost by component. Safe from any thread.
void recordTransaction(
    const facebook::react::TransactionTelemetry &telemetry,
    int mutations,
    std::chrono::steady_clock::time_point mountStart,
    std::chrono::steady_clock::time_point mountEnd,
    const ComponentMountCosts &components);

/// What was recorded since the last call, which starts a new window.
TransactionStats takeTransactionStats();

} // namespace rntizen
