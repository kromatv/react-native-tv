#include "ModuleDefinition.h"

#include "ExpoModuleRegistry.h"
#include "ExpoTestRuntime.test.h"

#include "threading/MainThread.h"

#include <doctest/doctest.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace rntizen::expo {

namespace {

using testing::ExpoTestRuntime;

struct Observed {
  std::mutex mutex;
  std::vector<std::string> calls;
  std::optional<Promise> held;
  std::atomic<bool> onMainThread{false};

  void add(std::string call) {
    std::lock_guard lock(mutex);
    calls.push_back(std::move(call));
  }

  std::vector<std::string> take() {
    std::lock_guard lock(mutex);
    return std::exchange(calls, {});
  }
};

Observed &observed() {
  static Observed state;
  return state;
}

class DefinitionTestModule final : public Module {
 public:
  ModuleDefinition definition() override {
    return ModuleDefinition("DefinitionTestModule")
        .constant("answer", [] { return 42; })
        .constant("label", [] { return std::string("forty-two"); })
        .property("counter", [this] { return ++reads_; })
        .function("add", [](int left, int right) { return left + right; })
        .function("join", [](const std::vector<std::string> &parts, const std::optional<std::string> &separator) {
          std::string joined;
          for (const auto &part : parts) joined += (joined.empty() ? "" : separator.value_or(",")) + part;
          return joined;
        })
        .function("nothing", [] {})
        .function("fail", [] { throw std::runtime_error("it broke"); })
        .function("failCoded", [] { throw CodedError("ERR_TEST", "it broke with a code"); })
        .function("emitTick", [this](int value) { sendEvent("tick", folly::dynamic::object("value", value)); })
        .asyncFunction("double", [](int value) { return value * 2; })
        .asyncFunction("forget", [](const std::string &entry) { observed().add("forget " + entry); })
        .asyncFunction("failLater", [] { throw CodedError("ERR_LATER", "it broke later"); })
        .asyncFunction("failPlain", []() -> int { throw std::runtime_error("plain failure"); })
        .asyncFunction(
            "onMain",
            [] {
              observed().onMainThread = isMainThread();
              return std::string("main");
            },
            Queue::Main)
        .asyncFunction("hold", [](const Promise &promise) { observed().held = promise; })
        .asyncFunction("resolveNow", [](int value, const Promise &promise) { promise.resolve(value + 1); })
        .asyncFunction("rejectTwice", [](const Promise &promise) {
          promise.reject("ERR_FIRST", "first");
          promise.reject("ERR_SECOND", "second");
        })
        .asyncFunction("throwWithPromise", [](const Promise &) { throw CodedError("ERR_BODY", "thrown before settling"); })
        .onStartObserving([](const std::string &event) { observed().add("start " + event); })
        .onStopObserving([](const std::string &event) { observed().add("stop " + event); });
  }

 private:
  int reads_{0};
};

class MisnamedTestModule final : public Module {
 public:
  ModuleDefinition definition() override {
    return ModuleDefinition("SomethingElse").constant("ready", [] { return true; });
  }
};

const bool registered = registerExpoModuleType<DefinitionTestModule>("DefinitionTestModule") &&
                        registerExpoModuleType<MisnamedTestModule>("MisnamedTestModule");

std::string rejection(ExpoTestRuntime &expo, const std::string &source) {
  try {
    expo.settle(source);
  } catch (const std::runtime_error &error) {
    return error.what();
  }
  return "resolved";
}

std::string thrown(ExpoTestRuntime &expo, const std::string &source) {
  return expo
      .eval("(() => { try { " + source + "; return 'no error'; } catch (e) { return (e.code ? e.code + ': ' : '') + e.message; } })()")
      .asString(expo.runtime())
      .utf8(expo.runtime());
}

} // namespace

TEST_CASE("A module definition gives JavaScript its constants, once, and its properties, on every read") {
  ExpoTestRuntime expo;
  CHECK(expo.json("[expo.modules.DefinitionTestModule.answer, expo.modules.DefinitionTestModule.label]") ==
        R"([42,"forty-two"])");
  CHECK(expo.json("[expo.modules.DefinitionTestModule.counter, expo.modules.DefinitionTestModule.counter]") == "[1,2]");
  CHECK(expo.eval("expo.modules.DefinitionTestModule === expo.modules.DefinitionTestModule").getBool());
  CHECK(expo.eval("expo.modules.DefinitionTestModule instanceof expo.NativeModule").getBool());
}

TEST_CASE("A module function converts its arguments and its result") {
  ExpoTestRuntime expo;
  CHECK(expo.json("expo.modules.DefinitionTestModule.add(2, 3)") == "5");
  CHECK(expo.json("expo.modules.DefinitionTestModule.join(['a', 'b'])") == "\"a,b\"");
  CHECK(expo.json("expo.modules.DefinitionTestModule.join(['a', 'b'], ' and ')") == "\"a and b\"");
  CHECK(expo.eval("expo.modules.DefinitionTestModule.nothing() === undefined").getBool());
  CHECK(thrown(expo, "expo.modules.DefinitionTestModule.add(2)") == "Expected a number");
  CHECK(thrown(expo, "expo.modules.DefinitionTestModule.add('2', 3)") == "Expected a number");
}

TEST_CASE("A module function that throws gives JavaScript an Error, with Expo's code for a CodedError") {
  ExpoTestRuntime expo;
  CHECK(thrown(expo, "expo.modules.DefinitionTestModule.fail()") == "it broke");
  CHECK(thrown(expo, "expo.modules.DefinitionTestModule.failCoded()") == "ERR_TEST: it broke with a code");
  CHECK(expo.eval("(() => { try { expo.modules.DefinitionTestModule.fail(); } catch (e) { return e instanceof Error; } })()")
            .getBool());
}

TEST_CASE("An async function runs off the JavaScript thread and settles its promise") {
  ExpoTestRuntime expo;
  CHECK(expo.eval("expo.modules.DefinitionTestModule.double(4) instanceof Promise").getBool());
  CHECK(expo.settle("expo.modules.DefinitionTestModule.double(21)") == "42");
  observed().take();
  CHECK(expo.settle("expo.modules.DefinitionTestModule.forget('this')") == "undefined");
  CHECK(observed().take() == std::vector<std::string>{"forget this"});
  CHECK(rejection(expo, "expo.modules.DefinitionTestModule.failLater()") == "ERR_LATER: it broke later");
  CHECK(rejection(expo, "expo.modules.DefinitionTestModule.failPlain()") == "plain failure");
  CHECK(rejection(expo, "expo.modules.DefinitionTestModule.double('four')") == "Expected a number");
}

TEST_CASE("An async function on the main queue runs on the EFL main loop") {
  ExpoTestRuntime expo;
  observed().onMainThread = false;
  CHECK(expo.settle("expo.modules.DefinitionTestModule.onMain()") == "\"main\"");
  CHECK(observed().onMainThread);
}

TEST_CASE("A host module's main-queue functions run on the main loop: ExpoKeepAwake keeps its tags without a window") {
  ExpoTestRuntime expo;
  CHECK(expo.settle("expo.modules.ExpoKeepAwake.isAvailableAsync()") == "false");
  CHECK(expo.settle("expo.modules.ExpoKeepAwake.activate('player')") == "false");
  CHECK(expo.settle("expo.modules.ExpoKeepAwake.isActivated()") == "true");
  CHECK(expo.settle("expo.modules.ExpoKeepAwake.deactivate('player')") == "false");
  CHECK(expo.settle("expo.modules.ExpoKeepAwake.isActivated()") == "false");
  CHECK(rejection(expo, "expo.modules.ExpoKeepAwake.activate(42)") == "Expected a string");
}

TEST_CASE("An async function with a Promise settles it when it chooses, and only the first time") {
  ExpoTestRuntime expo;
  CHECK(expo.settle("expo.modules.DefinitionTestModule.resolveNow(9)") == "10");
  CHECK(rejection(expo, "expo.modules.DefinitionTestModule.rejectTwice()") == "ERR_FIRST: first");
  CHECK(rejection(expo, "expo.modules.DefinitionTestModule.throwWithPromise()") == "ERR_BODY: thrown before settling");

  observed().held.reset();
  expo.eval(
      "globalThis.heldResult = undefined;"
      " expo.modules.DefinitionTestModule.hold().then(value => { globalThis.heldResult = value; })");
  REQUIRE(expo.pumpUntil([] { return observed().held.has_value(); }));
  const auto promise = *observed().held;
  std::thread([promise] { promise.resolve(std::string("from another thread")); }).join();
  REQUIRE(expo.pumpUntil([&expo] { return !expo.eval("globalThis.heldResult === undefined").getBool(); }));
  CHECK(expo.json("heldResult") == "\"from another thread\"");

  std::thread([promise] { promise.resolve(); }).join();
  expo.pump();
  CHECK(expo.json("heldResult") == "\"from another thread\"");
}

TEST_CASE("A module observes the first listener of an event and the removal of the last") {
  ExpoTestRuntime expo;
  observed().take();
  expo.eval(
      "globalThis.ticks = [];"
      " globalThis.first = expo.modules.DefinitionTestModule.addListener('tick', event => ticks.push(event.value));"
      " globalThis.second = expo.modules.DefinitionTestModule.addListener('tick', () => {});");
  CHECK(observed().take() == std::vector<std::string>{"start tick"});

  expo.eval("expo.modules.DefinitionTestModule.emitTick(3)");
  REQUIRE(expo.pumpUntil([&expo] { return expo.eval("ticks.length === 1").getBool(); }));
  CHECK(expo.json("ticks") == "[3]");

  expo.eval("first.remove()");
  CHECK(observed().take().empty());
  expo.eval("second.remove()");
  CHECK(observed().take() == std::vector<std::string>{"stop tick"});
}

TEST_CASE("A module that defines itself under another name still answers as the one registered") {
  ExpoTestRuntime expo;
  CHECK(expo.eval("expo.modules.MisnamedTestModule.ready").getBool());
}

} // namespace rntizen::expo
