#include "TextInputProps.h"

#include "Components.test.h"

#include <doctest/doctest.h>

#include <folly/dynamic.h>

#include <memory>
#include <optional>

namespace rntizen {

namespace {

std::shared_ptr<const TextInputProps> propsOf(const facebook::react::ShadowNode &node) {
  auto props = std::dynamic_pointer_cast<const TextInputProps>(node.getProps());
  REQUIRE(props != nullptr);
  return props;
}

} // namespace

TEST_CASE("TextInputProps reads the keyboard props Android parses") {
  testing::Components components;

  const auto node = components.create(
      "AndroidTextInput",
      folly::dynamic::object("secureTextEntry", true)("autoCorrect", true)("selectTextOnFocus", true)(
          "caretHidden", true)("showSoftInputOnFocus", false)("keyboardType", "numeric")("returnKeyType", "search"));

  const auto props = propsOf(*node);
  CHECK(props->secureTextEntry);
  CHECK(props->autoCorrect);
  CHECK(props->selectTextOnFocus);
  CHECK(props->caretHidden);
  CHECK(props->showSoftInputOnFocus == std::optional(false));
  CHECK(props->keyboardType == "numeric");
  CHECK(props->returnKeyType == "search");
}

TEST_CASE("TextInputProps leaves the keyboard props off and showSoftInputOnFocus unset unless given") {
  testing::Components components;

  const auto node = components.create("AndroidTextInput", folly::dynamic::object("text", "hello"));

  const auto props = propsOf(*node);
  CHECK_FALSE(props->secureTextEntry);
  CHECK_FALSE(props->autoCorrect);
  CHECK_FALSE(props->showSoftInputOnFocus.has_value());
  CHECK(props->keyboardType.empty());
  CHECK(props->returnKeyType.empty());
}

TEST_CASE("TextInputProps keeps the keyboard props an update leaves out") {
  testing::Components components;
  const auto node = components.create(
      "AndroidTextInput", folly::dynamic::object("keyboardType", "email-address")("showSoftInputOnFocus", true));

  const auto updated = components.update(*node, folly::dynamic::object("returnKeyType", "go"), nullptr);

  const auto props = propsOf(*updated);
  CHECK(props->keyboardType == "email-address");
  CHECK(props->showSoftInputOnFocus == std::optional(true));
  CHECK(props->returnKeyType == "go");
}

} // namespace rntizen
