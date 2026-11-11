#include "PerfSample.h"

#include <doctest/doctest.h>
#include <folly/json.h>

#include <string>

namespace rntizen {

TEST_CASE("perfLine lists the components that cost the most mount time first") {
  PerfSample sample;
  sample.transactions.components = {
      {"Text", {.mutations = 4, .ms = 0.5}}, {"Image", {.mutations = 1, .ms = 3}}, {"View", {.mutations = 9, .ms = 1}}};

  const auto line = perfLine(sample);

  REQUIRE(line.starts_with("perf "));
  const auto image = line.find("\"Image\"");
  const auto view = line.find("\"View\"");
  const auto text = line.find("\"Text\"");
  REQUIRE(image != std::string::npos);
  REQUIRE(view != std::string::npos);
  REQUIRE(text != std::string::npos);
  CHECK(image < view);
  CHECK(view < text);
}

TEST_CASE("perfLine carries the process's memory when it was read, and leaves it out when not") {
  PerfSample sample;
  sample.memory = {.pssMb = 142.04, .swapMb = 3.2, .source = "smaps_rollup"};
  const auto read = folly::parseJson(perfLine(sample).substr(5));
  sample.memory = {};
  const auto unread = folly::parseJson(perfLine(sample).substr(5));

  CHECK(read["pssMb"].asDouble() == 142.0);
  CHECK(read["swapMb"].asDouble() == 3.2);
  CHECK(read["memorySource"].asString() == "smaps_rollup");
  CHECK(unread.count("pssMb") == 0);
}

} // namespace rntizen
