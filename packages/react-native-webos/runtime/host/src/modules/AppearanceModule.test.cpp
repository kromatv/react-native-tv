#include "AppearanceModule.h"

#include "TurboModuleTest.test.h"

#include <doctest/doctest.h>

namespace rntizen {

using facebook::jsi::String;
using testing::call;

TEST_CASE("Appearance starts dark and keeps only light or dark") {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  AppearanceModule module(std::make_shared<testing::QueuedInvoker>(), ModuleContext{});
  auto scheme = [&] { return call(rt, module, "getColorScheme").asString(rt).utf8(rt); };

  CHECK(scheme() == "dark");
  call(rt, module, "setColorScheme", String::createFromUtf8(rt, "light"));
  CHECK(scheme() == "light");
  call(rt, module, "setColorScheme", String::createFromUtf8(rt, "unspecified"));
  CHECK(scheme() == "dark");
  CHECK(call(rt, module, "addListener", String::createFromUtf8(rt, "appearanceChanged")).isUndefined());
  CHECK(call(rt, module, "removeListeners", 1).isUndefined());
}

} // namespace rntizen
