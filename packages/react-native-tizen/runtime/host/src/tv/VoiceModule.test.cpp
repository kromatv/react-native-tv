#include "VoiceModule.h"

#include "TvState.h"

#include <doctest/doctest.h>

#include <hermes/hermes.h>

#include <memory>
#include <string>
#include <utility>

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

std::shared_ptr<VoiceModule> voiceModule(std::shared_ptr<TvState> tv, std::string platformVersion = "9.0") {
  ModuleContext context{};
  context.tv = std::move(tv);
  context.device.platformVersion = std::move(platformVersion);
  return std::make_shared<VoiceModule>(std::make_shared<NoInvoker>(), context);
}

} // namespace

TEST_CASE("RNTizenVoice reports availability, the screen and the listeners of each event") {
  auto runtime = facebook::hermes::makeHermesRuntime();
  auto &rt = *runtime;
  const auto tv = std::make_shared<TvState>(true, std::nullopt);
  const auto voice = voiceModule(tv);
  tv->setVoiceAvailable(true);

  CHECK(call(rt, *voice, "isAvailable").getBool());
  call(rt, *voice, "addListener", String::createFromAscii(rt, "search"));
  CHECK(tv->listening("search"));
  call(rt, *voice, "removeListener", String::createFromAscii(rt, "search"));
  CHECK_FALSE(tv->listening("search"));
  call(rt, *voice, "setScreen", String::createFromAscii(rt, "player"));
  CHECK(tv->screen() == "player");
  call(rt, *voice, "setScreen");
  CHECK_FALSE(tv->screen().has_value());
}

TEST_CASE("RNTizenVoice without the TV's state is unavailable and ignores the rest") {
  auto runtime = facebook::hermes::makeHermesRuntime();
  auto &rt = *runtime;
  const auto voice = voiceModule(nullptr);

  CHECK_FALSE(call(rt, *voice, "isAvailable").getBool());
  CHECK(call(rt, *voice, "unavailableReason").getString(rt).utf8(rt).starts_with("Samsung's voice service has not"));
  CHECK(call(rt, *voice, "setScreen", String::createFromAscii(rt, "home")).isUndefined());
  CHECK(call(rt, *voice, "addListener", String::createFromAscii(rt, "search")).isUndefined());
}

TEST_CASE("RNTizenVoice names the Tizen voice needs on an older TV") {
  auto runtime = facebook::hermes::makeHermesRuntime();
  auto &rt = *runtime;
  const auto tv = std::make_shared<TvState>(true, std::nullopt);
  tv->setVoiceAvailable(true);
  const auto voice = voiceModule(tv, "5.5");

  CHECK_FALSE(call(rt, *voice, "isAvailable").getBool());
  CHECK(call(rt, *voice, "unavailableReason").getString(rt).utf8(rt) == "Voice control needs Tizen 6.0, and this TV runs 5.5");
  CHECK_FALSE(call(rt, *voice, "canSelectTitles").getBool());
}

TEST_CASE("RNTizenVoice selects titles from Tizen 6.5") {
  auto runtime = facebook::hermes::makeHermesRuntime();
  auto &rt = *runtime;
  const auto tv = std::make_shared<TvState>(true, std::nullopt);
  tv->setVoiceAvailable(true);

  CHECK(call(rt, *voiceModule(tv, "6.0"), "isAvailable").getBool());
  CHECK(call(rt, *voiceModule(tv, "6.0"), "unavailableReason").isNull());
  CHECK_FALSE(call(rt, *voiceModule(tv, "6.0"), "canSelectTitles").getBool());
  CHECK(call(rt, *voiceModule(tv, "6.5"), "canSelectTitles").getBool());
}

} // namespace rntizen
