#include "DevLoadingViewModule.h"
#include "DevSettingsModule.h"
#include "LogBoxModule.h"

#include <doctest/doctest.h>

#include <hermes/hermes.h>

#include <string>

namespace rntizen {

namespace {

bool hasMethod(facebook::jsi::Runtime &runtime, facebook::react::TurboModule &module, const std::string &name) {
  for (const auto &property : module.getPropertyNames(runtime)) {
    if (property.utf8(runtime) == name) return true;
  }
  return false;
}

} // namespace

TEST_CASE("argbColor reads processColor's signed and unsigned colors alike") {
  CHECK_FALSE(argbColor(std::nullopt).has_value());
  CHECK(argbColor(4280648936.0) == 0xff2584e8U);
  CHECK(argbColor(-14318360.0) == 0xff2584e8U);
}

TEST_CASE("The dev support modules answer every call with no dev server, doing nothing") {
  auto runtime = facebook::hermes::makeHermesRuntime();
  DevSettingsModule settings(nullptr, ModuleContext{});
  LogBoxModule logBox(nullptr, ModuleContext{});
  DevLoadingViewModule loadingView(nullptr, ModuleContext{});

  settings.reload(*runtime);
  settings.reloadWithReason(*runtime, "test");
  settings.onFastRefresh(*runtime);
  settings.setHotLoadingEnabled(*runtime, false);
  settings.setProfilingEnabled(*runtime, true);
  settings.toggleElementInspector(*runtime);
  settings.addMenuItem(*runtime, "Item");
  settings.openDebugger(*runtime);
  settings.addListener(*runtime, "didPressMenuItem");
  settings.removeListeners(*runtime, 1);
  settings.setIsShakeToShowDevMenuEnabled(*runtime, true);
  logBox.show(*runtime);
  logBox.hide(*runtime);
  loadingView.showMessage(*runtime, "Refreshing...", 4294967295.0, 4280648936.0, false);
  loadingView.hide(*runtime);

  CHECK(hasMethod(*runtime, settings, "reloadWithReason"));
  CHECK(hasMethod(*runtime, logBox, "show"));
  CHECK(hasMethod(*runtime, loadingView, "showMessage"));
}

} // namespace rntizen
