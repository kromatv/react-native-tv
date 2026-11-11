#include "I18nManagerModule.h"

#include "TurboModuleTest.test.h"

#include <doctest/doctest.h>

namespace rnwebos {

using testing::call;

TEST_CASE("I18nManager reports the TV's menu language as a locale identifier, and never lays out right to left") {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  ModuleContext context{};
  context.device.locale = "en-GB";
  I18nManagerModule module(std::make_shared<testing::QueuedInvoker>(), context);

  CHECK(facebook::jsi::dynamicFromValue(rt, call(rt, module, "getConstants")) ==
        folly::dynamic::object("isRTL", false)("doLeftAndRightSwapInRTL", true)("localeIdentifier", "en_GB"));
  CHECK(call(rt, module, "allowRTL", true).isUndefined());
  CHECK(call(rt, module, "forceRTL", true).isUndefined());
  CHECK(call(rt, module, "swapLeftAndRightInRTL", false).isUndefined());
}

TEST_CASE("I18nManager leaves the locale identifier out when the host knows no language") {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  I18nManagerModule module(std::make_shared<testing::QueuedInvoker>(), ModuleContext{});

  CHECK(facebook::jsi::dynamicFromValue(rt, call(rt, module, "getConstants")) ==
        folly::dynamic::object("isRTL", false)("doLeftAndRightSwapInRTL", true));
}

} // namespace rnwebos
