#include "ModuleDefinition.h"

#include "ExpoModuleRegistry.h"
#include "ExpoTestRuntime.test.h"

#include <doctest/doctest.h>

#include <atomic>
#include <memory>
#include <string>

namespace rntizen::expo {

namespace {

using testing::ExpoTestRuntime;

std::atomic<int> &releasedCounters() {
  static std::atomic<int> released{0};
  return released;
}

class Counter final : public SharedObject {
 public:
  explicit Counter(int start) : value_(start) {}

  int value() const {
    return value_;
  }

  void set(int value) {
    value_ = value;
  }

  int add(int amount) {
    value_ += amount;
    return value_;
  }

  void sharedObjectDidRelease() override {
    ++releasedCounters();
  }

 private:
  int value_;
};

class ClassTestModule final : public Module {
 public:
  ModuleDefinition definition() override {
    return ModuleDefinition("ClassTestModule")
        .function("ping", [this](const std::shared_ptr<Counter> &counter) { sendEvent(counter, "ping", folly::dynamic::object("value", counter->value())); })
        .function("sameCounter", [](const std::shared_ptr<Counter> &counter) { return counter; })
        .function("newCounter", [](int start) { return std::make_shared<Counter>(start); })
        .sharedClass(ClassDefinition<Counter>("Counter")
                         .constructor([](const std::optional<int> &start) { return std::make_shared<Counter>(start.value_or(0)); })
                         .property("value", [](const Counter &counter) { return counter.value(); })
                         .property(
                             "settable",
                             [](const Counter &counter) { return counter.value(); },
                             [](Counter &counter, int value) { counter.set(value); })
                         .function("add", [](Counter &counter, int amount) { return counter.add(amount); })
                         .asyncFunction("addLater", [](Counter &counter, int amount) { return counter.add(amount); })
                         .asyncFunction("valueOnMain", [](const Counter &counter) { return counter.value(); }, Queue::Main));
  }
};

const bool registered = registerExpoModuleType<ClassTestModule>("ClassTestModule");

std::string rejection(ExpoTestRuntime &expo, const std::string &source) {
  try {
    expo.settle(source);
  } catch (const std::runtime_error &error) {
    return error.what();
  }
  return "resolved";
}

std::string thrown(ExpoTestRuntime &expo, const std::string &source) {
  return expo.eval("(() => { try { " + source + "; return 'no error'; } catch (e) { return e.message; } })()")
      .asString(expo.runtime())
      .utf8(expo.runtime());
}

} // namespace

TEST_CASE("A shared class constructs a native object JavaScript reads, writes and calls") {
  ExpoTestRuntime expo;
  expo.eval("const { Counter } = expo.modules.ClassTestModule; globalThis.counter = new Counter(5);");
  CHECK(expo.eval("counter instanceof expo.SharedObject").getBool());
  CHECK(expo.json("counter.value") == "5");
  CHECK(expo.json("counter.add(3)") == "8");
  expo.eval("counter.settable = 20");
  CHECK(expo.json("[counter.value, counter.settable]") == "[20,20]");
  CHECK(expo.json("new expo.modules.ClassTestModule.Counter().value") == "0");
  CHECK(expo.eval("counter.__expo_shared_object_id__ > 0").getBool());
}

TEST_CASE("A shared class runs its async functions with the native object on both queues") {
  ExpoTestRuntime expo;
  expo.eval("globalThis.counter = new expo.modules.ClassTestModule.Counter(1)");
  CHECK(expo.settle("counter.addLater(41)") == "42");
  CHECK(expo.settle("counter.valueOnMain()") == "42");
}

TEST_CASE("A JavaScript subclass of a shared class reaches native code through super()") {
  ExpoTestRuntime expo;
  expo.eval(
      "class Doubling extends expo.modules.ClassTestModule.Counter {"
      "  constructor(start) { super(start * 2); }"
      "  twice() { return this.add(this.value); }"
      "}"
      "globalThis.doubling = new Doubling(3);");
  CHECK(expo.json("[doubling.value, doubling.twice()]") == "[6,12]");
  CHECK(expo.eval("doubling instanceof expo.modules.ClassTestModule.Counter").getBool());
}

TEST_CASE("A native object returned to JavaScript is the instance it already has, or a new one of its class") {
  ExpoTestRuntime expo;
  expo.eval("globalThis.counter = new expo.modules.ClassTestModule.Counter(2)");
  CHECK(expo.eval("expo.modules.ClassTestModule.sameCounter(counter) === counter").getBool());
  CHECK(expo.eval("expo.modules.ClassTestModule.sameCounter(counter.__expo_shared_object_id__) === counter").getBool());
  CHECK(expo.json("expo.modules.ClassTestModule.newCounter(7).value") == "7");
  CHECK(expo.eval("expo.modules.ClassTestModule.newCounter(7) instanceof expo.modules.ClassTestModule.Counter").getBool());
}

TEST_CASE("A released shared object refuses its methods and frees the native object") {
  ExpoTestRuntime expo;
  const auto before = releasedCounters().load();
  expo.eval("globalThis.counter = new expo.modules.ClassTestModule.Counter(4); globalThis.id = counter.__expo_shared_object_id__; counter.release();");
  CHECK(releasedCounters() == before + 1);
  CHECK(expo.json("counter.__expo_shared_object_id__") == "0");
  CHECK(thrown(expo, "counter.add(1)") == "The native object was released or is of another class");
  CHECK(thrown(expo, "expo.modules.ClassTestModule.sameCounter(id)") == "Expected a live shared object of the right class");
  CHECK(thrown(expo, "expo.modules.ClassTestModule.Counter.prototype.add.call({}, 1)") ==
        "The native object was released or is of another class");
  CHECK(rejection(expo, "counter.addLater(1)") == "The native object was released or is of another class");
}

TEST_CASE("Finalizing a shared object's JavaScript instance frees its native object") {
  const auto before = releasedCounters().load();
  {
    ExpoTestRuntime expo;
    expo.eval("(() => { for (let i = 0; i < 8; ++i) new expo.modules.ClassTestModule.Counter(i); })()");
  }
  CHECK(releasedCounters() == before + 8);
}

TEST_CASE("A native event reaches the JavaScript instance paired with the object, and not a released one") {
  ExpoTestRuntime expo;
  expo.eval(
      "globalThis.pings = [];"
      " globalThis.counter = new expo.modules.ClassTestModule.Counter(11);"
      " counter.addListener('ping', event => pings.push(event.value));"
      " expo.modules.ClassTestModule.ping(counter);");
  REQUIRE(expo.pumpUntil([&expo] { return expo.eval("pings.length === 1").getBool(); }));
  CHECK(expo.json("pings") == "[11]");

  auto counter = Convert<std::shared_ptr<Counter>>::fromJs(expo.runtime(), expo.eval("counter"));
  expo.eval("counter.release()");
  expo.context()->emit(counter, "ping", dynamicPayload(folly::dynamic::object("value", 12)));
  for (int round = 0; round < 5; ++round) expo.pump();
  CHECK(expo.json("pings") == "[11]");
}

} // namespace rntizen::expo
