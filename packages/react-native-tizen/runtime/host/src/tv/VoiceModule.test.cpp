#include "VoiceModule.h"

#include "TvState.h"

#include <doctest/doctest.h>

#include <hermes/hermes.h>

#include <memory>
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

std::shared_ptr<VoiceModule> voiceModule(std::shared_ptr<TvState> tv) {
  ModuleContext context{};
  context.tv = std::move(tv);
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
  CHECK(call(rt, *voice, "setScreen", String::createFromAscii(rt, "home")).isUndefined());
  CHECK(call(rt, *voice, "addListener", String::createFromAscii(rt, "search")).isUndefined());
}

} // namespace rntizen
