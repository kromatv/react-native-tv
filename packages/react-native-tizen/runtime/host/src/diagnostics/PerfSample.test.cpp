#include "PerfSample.h"

#include <doctest/doctest.h>

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

} // namespace rntizen
