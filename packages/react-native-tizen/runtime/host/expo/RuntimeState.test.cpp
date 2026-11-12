#include "RuntimeState.h"

#include "ExpoModuleRegistry.h"
#include "ExpoTestRuntime.test.h"
#include "ModuleDefinition.h"

#include <doctest/doctest.h>

#include <hermes/hermes.h>
#include <jsi/instrumentation.h>

#include <stdexcept>
#include <string>

namespace rntizen::expo {

namespace {

using facebook::jsi::JSError;
using facebook::jsi::Runtime;
using facebook::jsi::String;
using facebook::jsi::Value;
using testing::ExpoTestRuntime;

class Token final : public SharedObject {};

class StateTestModule final : public Module {
 public:
  ModuleDefinition definition() override {
    return ModuleDefinition("StateTestModule")
        .sharedClass(ClassDefinition<Token>("Token").constructor([] { return std::make_shared<Token>(); }));
  }
};

const bool registered = registerExpoModuleType<StateTestModule>("StateTestModule");

std::string settleDirectly(ExpoTestRuntime &expo, PromiseOutcome outcome) {
  auto &runtime = expo.runtime();
  auto &state = RuntimeState::get(runtime);
  auto created = state.createPromise(runtime);
  runtime.global().setProperty(runtime, "__direct", created.promise);
  state.settle(runtime, created.id, outcome);
  try {
    return expo.settle("__direct");
  } catch (const std::runtime_error &error) {
    return std::string("rejected ") + error.what();
  }
}

} // namespace

TEST_CASE("RuntimeState refuses a runtime Expo was not installed in") {
  auto runtime = facebook::hermes::makeHermesRuntime();
  try {
    RuntimeState::get(*runtime);
    FAIL("RuntimeState::get found a state");
  } catch (const JSError &error) {
    CHECK(error.getMessage() == "Expo modules are not installed in this runtime");
  }
}

TEST_CASE("RuntimeState settles a promise with a value, an error and its code, or what building the value threw") {
  ExpoTestRuntime expo;
  CHECK(settleDirectly(expo, {.value = [](Runtime &rt) { return Value(String::createFromAscii(rt, "done")); }}) == "\"done\"");
  CHECK(settleDirectly(expo, {}) == "undefined");
  CHECK(settleDirectly(expo, {.value = nullptr, .error = "refused", .code = "ERR_NO"}) == "rejected ERR_NO: refused");
  CHECK(settleDirectly(expo, {.value = nullptr, .error = "no code", .code = ""}) == "rejected no code");
  CHECK(settleDirectly(expo, {.value = [](Runtime &rt) -> Value { throw JSError(rt, "from JavaScript"); }}) ==
        "rejected from JavaScript");
  CHECK(settleDirectly(expo, {.value = [](Runtime &) -> Value { throw std::runtime_error("from native code"); }}) ==
        "rejected from native code");
}

TEST_CASE("RuntimeState settles each promise once and ignores an id it does not know") {
  ExpoTestRuntime expo;
  auto &runtime = expo.runtime();
  auto &state = RuntimeState::get(runtime);
  auto created = state.createPromise(runtime);
  runtime.global().setProperty(runtime, "__once", created.promise);
  state.settle(runtime, created.id, {.value = [](Runtime &) { return Value(1); }});
  state.settle(runtime, created.id, {.value = [](Runtime &) { return Value(2); }});
  state.settle(runtime, created.id + 1000, {.value = nullptr, .error = "nobody waits", .code = ""});
  CHECK(expo.settle("__once") == "1");
}

TEST_CASE("RuntimeState gives back the instance of a live object and forgets a collected one") {
  ExpoTestRuntime expo;
  auto &runtime = expo.runtime();
  auto &state = RuntimeState::get(runtime);
  expo.eval("globalThis.token = new expo.modules.StateTestModule.Token()");
  const auto id = static_cast<SharedObjectId>(expo.eval("token.__expo_shared_object_id__").getNumber());
  REQUIRE(state.liveObject(runtime, id).has_value());
  CHECK(Value(runtime, *state.liveObject(runtime, id)).getObject(runtime).isFunction(runtime) == false);
  CHECK_FALSE(state.liveObject(runtime, id + 100000).has_value());

  expo.eval("delete globalThis.token");
  runtime.instrumentation().collectGarbage("test");
  CHECK_FALSE(state.liveObject(runtime, id).has_value());
}

TEST_CASE("RuntimeState drops a collected object even when another runtime took its release") {
  ExpoTestRuntime first;
  ExpoTestRuntime second;
  auto &runtime = first.runtime();
  auto &state = RuntimeState::get(runtime);
  first.eval("globalThis.token = new expo.modules.StateTestModule.Token()");
  const auto id = static_cast<SharedObjectId>(first.eval("token.__expo_shared_object_id__").getNumber());
  first.eval("delete globalThis.token");
  runtime.instrumentation().collectGarbage("test");
  CHECK_FALSE(RuntimeState::get(second.runtime()).liveObject(second.runtime(), id).has_value());
  CHECK_FALSE(state.liveObject(runtime, id).has_value());
  CHECK_FALSE(state.liveObject(runtime, id).has_value());
}

} // namespace rntizen::expo
