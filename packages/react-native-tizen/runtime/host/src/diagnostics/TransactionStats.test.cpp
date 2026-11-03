#include "TransactionStats.h"

#include <doctest/doctest.h>

#include <chrono>

namespace rntizen {

TEST_CASE("recordTransaction adds up a window's transactions and their cost by component") {
  setTransactionStatsEnabled(true);
  takeTransactionStats();
  const facebook::react::TransactionTelemetry telemetry;
  const auto start = std::chrono::steady_clock::now();

  recordTransaction(telemetry, 3, start, start + std::chrono::milliseconds(4), {{"View", {.mutations = 2, .ms = 1.5}}});
  recordTransaction(
      telemetry,
      1,
      start,
      start + std::chrono::milliseconds(2),
      {{"View", {.mutations = 1, .ms = 0.5}}, {"Text", {.mutations = 1, .ms = 0.25}}});
  const auto stats = takeTransactionStats();
  setTransactionStatsEnabled(false);

  CHECK(stats.count == 2);
  CHECK(stats.mutations == 4);
  CHECK(stats.mountMax == doctest::Approx(4));
  CHECK(stats.mountTotal == doctest::Approx(6));
  CHECK(stats.components.at("View").mutations == 3);
  CHECK(stats.components.at("View").ms == doctest::Approx(2));
  CHECK(stats.components.at("Text").mutations == 1);
  CHECK(takeTransactionStats().count == 0);
}

TEST_CASE("recordTransaction keeps nothing while disabled") {
  setTransactionStatsEnabled(false);
  const auto start = std::chrono::steady_clock::now();

  recordTransaction(facebook::react::TransactionTelemetry{}, 1, start, start, {});

  CHECK(takeTransactionStats().count == 0);
}

} // namespace rntizen
