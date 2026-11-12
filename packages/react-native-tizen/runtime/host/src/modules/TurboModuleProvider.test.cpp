#include "TurboModuleProvider.h"

#include "DeviceEventManagerModule.h"
#include "PlatformModuleRegistry.h"
#include "SourceCodeModule.h"
#include "StatusBarManagerModule.h"
#include "TurboModuleTest.test.h"
#include "VibrationModule.h"

#include "../logging/Log.h"

#include <ReactCommon/CxxTurboModuleUtils.h>
#include <doctest/doctest.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace rntizen {

namespace {

using facebook::jsi::String;
using testing::call;

class RecordingSink final : public LogSink {
 public:
  void write(LogLevel, std::string_view message) override {
    std::lock_guard lock(mutex);
    lines.emplace_back(message);
  }

  std::mutex mutex;
  std::vector<std::string> lines;
};

class NamedModule final : public facebook::react::TurboModule {
 public:
  explicit NamedModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker)
      : TurboModule("TestCxxModule", std::move(jsInvoker)) {}
};

} // namespace

TEST_CASE("the provider answers this platform's modules, React Native's defaults, then libraries' C++ modules") {
  auto invoker = std::make_shared<testing::QueuedInvoker>();
  ModuleContext context{};
  context.scriptURL = "http://127.0.0.1:8081/index.bundle";
  SourceCodeModule pulledIn(invoker, context);
  facebook::react::registerCxxModuleToGlobalModuleMap(
      "TestCxxModule", [](std::shared_ptr<facebook::react::CallInvoker> jsInvoker) {
        return std::make_shared<NamedModule>(std::move(jsInvoker));
      });
  auto sink = std::make_shared<RecordingSink>();
  setLogSink(sink);
  TurboModuleProvider provider(context, invoker);

  const auto sourceCode = provider.get("SourceCode");
  const auto microtasks = provider.get("NativeMicrotasksCxx");
  const auto library = provider.get("TestCxxModule");
  const auto missing = provider.get("NoSuchModule");
  const auto missingAgain = provider.get("NoSuchModule");
  setLogSink(nullptr);

  REQUIRE(sourceCode != nullptr);
  CHECK(provider.get("SourceCode") == sourceCode);
  CHECK(microtasks != nullptr);
  CHECK(std::dynamic_pointer_cast<NamedModule>(library) != nullptr);
  CHECK(missing == nullptr);
  CHECK(missingAgain == nullptr);
  CHECK(std::count(sink->lines.begin(), sink->lines.end(), "No native module named NoSuchModule") == 1);

  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  CHECK(testing::json(rt, call(rt, *sourceCode, "getConstants")) == R"({"scriptURL":"http://127.0.0.1:8081/index.bundle"})");
}

TEST_CASE("a platform module registers by name, and a later registration replaces it") {
  CHECK_FALSE(findPlatformModule("TestPlatformModule").has_value());
  registerPlatformModule("TestPlatformModule", [](auto, const ModuleContext &) { return nullptr; });
  registerPlatformModuleType<VibrationModule>("TestPlatformModule");

  const auto factory = findPlatformModule("TestPlatformModule");
  REQUIRE(factory.has_value());
  CHECK(std::dynamic_pointer_cast<VibrationModule>((*factory)(std::make_shared<testing::QueuedInvoker>(), ModuleContext{})) !=
        nullptr);
}

TEST_CASE("StatusBarManager has no height, and Vibration and the status bar's setters do nothing on a TV") {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  auto invoker = std::make_shared<testing::QueuedInvoker>();
  StatusBarManagerModule statusBar(invoker, ModuleContext{});
  VibrationModule vibration(invoker, ModuleContext{});

  CHECK(testing::json(rt, call(rt, statusBar, "getConstants")) == R"({"DEFAULT_BACKGROUND_COLOR":0,"HEIGHT":0})");
  CHECK(call(rt, statusBar, "setStyle", String::createFromUtf8(rt, "light-content")).isUndefined());
  CHECK(call(rt, statusBar, "setHidden", true).isUndefined());
  CHECK(testing::json(rt, call(rt, vibration, "getConstants")) == "{}");
  CHECK(call(rt, vibration, "vibrate", 400).isUndefined());
  CHECK(call(rt, vibration, "vibrateByPattern", testing::evaluate(rt, "[0, 100]"), -1).isUndefined());
  CHECK(call(rt, vibration, "cancel").isUndefined());
}

TEST_CASE("the default back press handler leaves the app") {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  int exits = 0;
  ModuleContext context{};
  context.exitApp = [&exits] { ++exits; };
  DeviceEventManagerModule module(std::make_shared<testing::QueuedInvoker>(), context);

  call(rt, module, "invokeDefaultBackPressHandler");
  CHECK(exits == 1);
}

} // namespace rntizen
