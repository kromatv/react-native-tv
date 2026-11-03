#include "PerfSample.h"

#include <folly/dynamic.h>
#include <folly/json.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace rntizen {

namespace {

constexpr std::size_t kMountTopSize = 3;

double tenths(double value) {
  return std::round(value * 10) / 10;
}

double average(double total, int count) {
  return count > 0 ? total / count : 0;
}

folly::dynamic mountTop(const TransactionStats &transactions) {
  std::vector<std::pair<std::string, ComponentMountCost>> costs(
      transactions.components.begin(), transactions.components.end());
  std::ranges::sort(costs, [](const auto &left, const auto &right) { return left.second.ms > right.second.ms; });
  folly::dynamic top = folly::dynamic::array;
  for (std::size_t index = 0; index < costs.size() && index < kMountTopSize; ++index) {
    const auto &[name, cost] = costs[index];
    top.push_back(folly::dynamic::object("name", name)("ms", tenths(cost.ms))("mutations", cost.mutations));
  }
  return top;
}

} // namespace

std::string perfLine(const PerfSample &sample) {
  const auto &frames = sample.frames;
  const auto &transactions = sample.transactions;
  folly::dynamic fields = folly::dynamic::object;
  fields["t"] = static_cast<std::int64_t>(std::llround(sample.uptimeMs));
  fields["window"] = static_cast<std::int64_t>(std::llround(sample.windowMs));
  fields["frames"] = frames.frames;
  fields["ticks"] = frames.ticks;
  fields["hz"] = tenths(frames.periodMs > 0 ? 1000 / frames.periodMs : 0);
  fields["dropped"] = frames.dropped;
  fields["renderAvg"] = tenths(frames.renderAvgMs);
  fields["renderMax"] = tenths(frames.renderMaxMs);
  fields["mainBusy"] = tenths(frames.mainBusyMs);
  fields["jsBusy"] = tenths(sample.jsBusyMs);
  fields["jsWait"] = tenths(sample.jsWaitMs);
  fields["jsLag"] = tenths(sample.jsLagMs);
  fields["commits"] = transactions.count;
  fields["commitAvg"] = tenths(average(transactions.commitTotal, transactions.count));
  fields["commitMax"] = tenths(transactions.commitMax);
  fields["layoutMax"] = tenths(transactions.layoutMax);
  fields["mutations"] = transactions.mutations;
  fields["mountAvg"] = tenths(average(transactions.mountTotal, transactions.count));
  fields["mountMax"] = tenths(transactions.mountMax);
  fields["mountLatencyMax"] = tenths(transactions.latencyMax);
  fields["mountTop"] = mountTop(transactions);
  fields["heapUsedMb"] = tenths(sample.heap.usedMb);
  fields["heapSizeMb"] = tenths(sample.heap.sizeMb);
  fields["gcs"] = sample.heap.collections;
  fields["cpu"] = tenths(sample.cpuPercent);
  return "perf " + folly::toJson(fields);
}

} // namespace rntizen
