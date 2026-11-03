#include "UnwrappedSizes.h"

#include <doctest/doctest.h>

#include <string>

namespace rntizen {

namespace {

UnwrappedSizes::Key keyFor(const std::string &text, facebook::react::Float scale = 1) {
  facebook::react::AttributedString string;
  facebook::react::AttributedString::Fragment fragment;
  fragment.string = text;
  string.appendFragment(std::move(fragment));
  return {.attributedString = string, .paragraphAttributes = {}, .layoutConstraints = {}, .pointScaleFactor = scale};
}

constexpr facebook::react::Size kSize{.width = 120, .height = 30};

} // namespace

TEST_CASE("UnwrappedSizes knows nothing it was not told") {
  const UnwrappedSizes sizes;

  CHECK_FALSE(sizes.find(keyFor("Dune"), 500).has_value());
}

TEST_CASE("UnwrappedSizes answers for any width at least as wide as the widest line") {
  UnwrappedSizes sizes;

  sizes.remember(keyFor("Dune"), 240, kSize);

  CHECK(sizes.find(keyFor("Dune"), 240) == kSize);
  CHECK(sizes.find(keyFor("Dune"), 1920) == kSize);
  CHECK_FALSE(sizes.find(keyFor("Dune"), 239).has_value());
}

TEST_CASE("UnwrappedSizes keeps other text and other scales apart") {
  UnwrappedSizes sizes;

  sizes.remember(keyFor("Dune", 1), 240, kSize);

  CHECK_FALSE(sizes.find(keyFor("Arrival", 1), 1920).has_value());
  CHECK_FALSE(sizes.find(keyFor("Dune", 2), 1920).has_value());
}

TEST_CASE("UnwrappedSizes takes the latest size for a paragraph") {
  UnwrappedSizes sizes;
  sizes.remember(keyFor("Dune"), 240, kSize);

  sizes.remember(keyFor("Dune"), 100, {.width = 50, .height = 60});

  CHECK(sizes.find(keyFor("Dune"), 100) == facebook::react::Size{.width = 50, .height = 60});
}

} // namespace rntizen
