#include "MicButtonModule.h"

#include "../input/MicButton.h"

#include <doctest/doctest.h>

#include <hermes/hermes.h>

#include <memory>
#include <utility>
#include <vector>

namespace rntizen {

namespace {

using facebook::jsi::Runtime;
using facebook::jsi::String;
using facebook::jsi::Value;

class NoInvoker final : public facebook::react::CallInvoker {
 public:
  void invokeAsync(facebook::react::CallFunc &&) noexcept override {}
  void invokeSync(facebook::react::CallFunc &&) override {}
};

template <typename... Arguments>
Value call(Runtime &runtime, facebook::react::TurboModule &module, const char *method, Arguments &&...arguments) {
  auto function = module.get(runtime, facebook::jsi::PropNameID::forAscii(runtime, method)).asObject(runtime).asFunction(runtime);
  return function.call(runtime, std::forward<Arguments>(arguments)...);
}

std::shared_ptr<MicButtonModule> micButtonModule(std::shared_ptr<MicButton> button) {
  ModuleContext context{};
  context.micButton = std::move(button);
  return std::make_shared<MicButtonModule>(std::make_shared<NoInvoker>(), context);
}

} // namespace

TEST_CASE("RNTizenMicButton holds the button while JavaScript listens to it") {
  auto runtime = facebook::hermes::makeHermesRuntime();
  auto &rt = *runtime;
  std::vector<bool> grabs;
  const auto button =
      std::make_shared<MicButton>([](std::string_view) {}, [](std::function<void()> task) { task(); });
  button->setGrab([&grabs](bool grabbed) { grabs.push_back(grabbed); });
  const auto module = micButtonModule(button);

  CHECK(call(rt, *module, "isAvailable").getBool());
  call(rt, *module, "addListener", String::createFromAscii(rt, "pressIn"));
  call(rt, *module, "addListener", String::createFromAscii(rt, "pressOut"));
  call(rt, *module, "removeListener", String::createFromAscii(rt, "pressIn"));
  CHECK(grabs == std::vector<bool>{true});
  call(rt, *module, "removeListener", String::createFromAscii(rt, "pressOut"));

  CHECK(grabs == std::vector<bool>{true, false});
}

TEST_CASE("RNTizenMicButton without the host's button is unavailable and ignores listeners") {
  auto runtime = facebook::hermes::makeHermesRuntime();
  auto &rt = *runtime;
  const auto module = micButtonModule(nullptr);

  call(rt, *module, "addListener", String::createFromAscii(rt, "pressIn"));
  call(rt, *module, "removeListener", String::createFromAscii(rt, "pressIn"));

  CHECK_FALSE(call(rt, *module, "isAvailable").getBool());
}

} // namespace rntizen
