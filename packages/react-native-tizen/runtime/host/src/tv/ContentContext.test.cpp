#include "ContentContext.h"

#include <doctest/doctest.h>

#include <vector>

namespace rntizen {

namespace {

LabelledView labelled(const char *label, facebook::react::Float x, facebook::react::Float y) {
  return {.view = nullptr, .label = label, .frame = {.origin = {.x = x, .y = y}, .size = {.width = 200, .height = 100}}};
}

} // namespace

TEST_CASE("findByTitle prefers an exact label, then a prefix, then a part of one") {
  const std::vector views{
      labelled("The Dune Saga", 0, 0), labelled("Dune: Part Two", 200, 0), labelled("dune", 400, 0)};

  CHECK(findByTitle(views, "DUNE")->label == "dune");
  CHECK(findByTitle(views, "dune part")->label == "Dune: Part Two");
  CHECK(findByTitle(views, "saga")->label == "The Dune Saga");
}

TEST_CASE("findByTitle finds nothing for a title with no letters or no match") {
  const std::vector views{labelled("Dune", 0, 0)};

  CHECK(findByTitle(views, "?!") == nullptr);
  CHECK(findByTitle(views, "Arrival") == nullptr);
}

TEST_CASE("contentContext places each title at its view's center") {
  const std::vector views{labelled("Dune", 100, 50)};

  const auto items = contentContext(views);

  REQUIRE(items.size() == 1);
  CHECK(items[0]["positionX"] == 200);
  CHECK(items[0]["positionY"] == 100);
  CHECK(items[0]["title"] == "Dune");
  CHECK(items[0]["bFocused"] == false);
}

} // namespace rntizen
