#include "Context.h"

#include "ExpoModuleRegistry.h"
#include "ExpoTestRuntime.test.h"
#include "ModuleDefinition.h"

#include <doctest/doctest.h>

#include <stdexcept>
#include <string>

namespace rntizen::expo {

namespace {

using facebook::jsi::Runtime;
using testing::ExpoTestRuntime;

class Beacon final : public SharedObject {};

class ContextTestModule final : public Module {
 public:
  ModuleDefinition definition() override {
    return ModuleDefinition("ContextTestModule").constant("ready", [] { return true; });
  }
};

const bool registered = registerExpoModuleType<ContextTestModule>("ContextTestModule");

} // namespace

TEST_CASE("Context emits to a module only once JavaScript loaded it, and never to an unpaired object") {
  ExpoTestRuntime expo;
  expo.context()->emit("ContextTestModule", "early", dynamicPayload(folly::dynamic::object("seen", true)));
  for (int round = 0; round < 3; ++round) expo.pump();
  expo.eval("globalThis.events = []; expo.modules.ContextTestModule.addListener('early', e => events.push(e))");
  for (int round = 0; round < 3; ++round) expo.pump();
  CHECK(expo.json("events") == "[]");

  expo.context()->emit("ContextTestModule", "early", dynamicPayload(folly::dynamic::object("seen", true)));
  REQUIRE(expo.pumpUntil([&expo] { return expo.eval("events.length === 1").getBool(); }));
  CHECK(expo.json("events") == R"([{"seen":true}])");

  expo.context()->emit("ContextTestModule", "early", nullptr);
  REQUIRE(expo.pumpUntil([&expo] { return expo.eval("events.length === 2").getBool(); }));
  CHECK(expo.eval("events[1] === undefined").getBool());

  expo.context()->emit(std::make_shared<Beacon>(), "early", dynamicPayload(folly::dynamic::object()));
  for (int round = 0; round < 3; ++round) expo.pump();
  CHECK(expo.json("events.length") == "2");
}

TEST_CASE("Context logs what a JavaScript-thread task throws instead of passing it on") {
  ExpoTestRuntime expo;
  bool ran = false;
  expo.context()->runOnJs([](Runtime &) { throw std::runtime_error("a callback failed"); });
  expo.context()->runOnJs([&ran](Runtime &) { ran = true; });
  CHECK_NOTHROW(expo.pump());
  CHECK(ran);
}

TEST_CASE("Context knows the host and the resource directory, and no module it was not given") {
  ModuleContext host;
  host.scriptURL = "file:///bundle.js";
  ExpoTestRuntime expo(std::move(host));
  CHECK(expo.context()->host().scriptURL == "file:///bundle.js");
  CHECK(expo.context()->resourceDirectory() == (testing::testAppRoot() / "res").string() + "/");
  CHECK_FALSE(expo.context()->createModuleObject(expo.runtime(), "NoSuchModule").has_value());
  CHECK(expo.eval("expo.modules.NoSuchModule === undefined").getBool());
}

TEST_CASE("Context builds a module once and gives a second runtime's object the same instance") {
  ExpoTestRuntime expo;
  auto &runtime = expo.runtime();
  auto first = expo.context()->createModuleObject(runtime, "ContextTestModule");
  auto second = expo.context()->createModuleObject(runtime, "ContextTestModule");
  REQUIRE(first.has_value());
  REQUIRE(second.has_value());
  CHECK_FALSE(facebook::jsi::Object::strictEquals(runtime, *first, *second));
  CHECK(second->getProperty(runtime, "ready").getBool());
  CHECK(RuntimeState::get(runtime).context() == expo.context());
}

} // namespace rntizen::expo
