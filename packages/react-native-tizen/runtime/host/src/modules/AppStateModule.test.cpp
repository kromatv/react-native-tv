#include "AppStateModule.h"

#include "TurboModuleTest.test.h"

#include <doctest/doctest.h>

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace rntizen {

using facebook::jsi::String;
using testing::call;

TEST_CASE("AppState answers whether the app is in the foreground, at start and when asked") {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  ModuleContext context{};
  context.isActive = std::make_shared<std::atomic<bool>>(true);
  AppStateModule module(std::make_shared<testing::QueuedInvoker>(), context);
  auto successes = std::make_shared<std::vector<std::string>>();
  auto failures = std::make_shared<std::vector<std::string>>();

  CHECK(testing::json(rt, call(rt, module, "getConstants")) == R"({"initialAppState":"active"})");
  context.isActive->store(false);
  call(rt, module, "getCurrentAppState", testing::recorder(rt, successes), testing::recorder(rt, failures));
  context.isActive->store(true);
  call(rt, module, "getCurrentAppState", testing::recorder(rt, successes), testing::recorder(rt, failures));

  CHECK(*successes == std::vector<std::string>{R"({"app_state":"background"})", R"({"app_state":"active"})"});
  CHECK(failures->empty());
  CHECK(call(rt, module, "addListener", String::createFromUtf8(rt, "appStateDidChange")).isUndefined());
  CHECK(call(rt, module, "removeListeners", 1).isUndefined());
}

} // namespace rntizen
