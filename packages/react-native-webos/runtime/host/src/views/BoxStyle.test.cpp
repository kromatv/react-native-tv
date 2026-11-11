#include "BoxStyle.h"

#include "ViewTesting.test.h"

#include <doctest/doctest.h>

namespace rnwebos {

namespace {

using facebook::react::BorderStyle;
using facebook::react::ViewProps;

StrokePattern patternOf(void (*change)(ViewProps &)) {
  ViewProps props;
  change(props);
  return resolveBoxStyle(props, testing::frameAt(0, 0, 100, 100), false).borderPattern;
}

} // namespace

TEST_CASE("a box strokes its border as its one border style says, solid when it names none") {
  CHECK(patternOf([](ViewProps &) {}) == StrokePattern::Solid);
  CHECK(patternOf([](ViewProps &props) { props.borderStyles.all = BorderStyle::Solid; }) == StrokePattern::Solid);
  CHECK(patternOf([](ViewProps &props) { props.borderStyles.all = BorderStyle::Dashed; }) == StrokePattern::Dashed);
  CHECK(patternOf([](ViewProps &props) { props.borderStyles.all = BorderStyle::Dotted; }) == StrokePattern::Dotted);
}

TEST_CASE("a box whose edges differ in border style strokes them all solid") {
  CHECK(patternOf([](ViewProps &props) {
          props.borderStyles.all = BorderStyle::Dashed;
          props.borderStyles.left = BorderStyle::Dotted;
        }) == StrokePattern::Solid);
}

} // namespace rnwebos
