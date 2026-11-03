#include "PreviewModule.h"

#include "TvState.h"

#include <doctest/doctest.h>

#include <Ecore.h>
#include <hermes/hermes.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace rntizen {

namespace {

using facebook::jsi::Function;
using facebook::jsi::PropNameID;
using facebook::jsi::Runtime;
using facebook::jsi::String;
using facebook::jsi::Value;

class QueuedInvoker final : public facebook::react::CallInvoker {
 public:
  void invokeAsync(facebook::react::CallFunc &&task) noexcept override {
    tasks.push_back(std::move(task));
  }
  void invokeSync(facebook::react::CallFunc &&task) override {
    tasks.push_back(std::move(task));
  }

  void run(Runtime &runtime) {
    for (auto &task : std::exchange(tasks, {})) task(runtime);
  }

  std::vector<facebook::react::CallFunc> tasks;
};

template <typename... Arguments>
Value call(Runtime &runtime, facebook::react::TurboModule &module, const char *method, Arguments &&...arguments) {
  auto function = module.get(runtime, PropNameID::forAscii(runtime, method)).asObject(runtime).asFunction(runtime);
  return function.call(runtime, std::forward<Arguments>(arguments)...);
}

void runMainLoopBriefly() {
  ecore_timer_add(
      0.05,
      [](void *) -> Eina_Bool {
        ecore_main_loop_quit();
        return ECORE_CALLBACK_CANCEL;
      },
      nullptr);
  ecore_main_loop_begin();
}

struct Preview {
  Preview() {
    ecore_init();
    ModuleContext context{};
    context.tv = tv;
    module = std::make_shared<PreviewModule>(invoker, context);
  }
  ~Preview() {
    ecore_shutdown();
  }
  Preview(const Preview &) = delete;
  Preview &operator=(const Preview &) = delete;

  std::string publish(const char *json) {
    auto &rt = *runtime;
    auto promise = call(rt, *module, "publish", String::createFromAscii(rt, json)).asObject(rt);
    std::string outcome;
    auto record = [&outcome](const char *prefix) {
      return [&outcome, prefix](Runtime &rt, const Value &, const Value *arguments, size_t) {
        const auto &value = arguments[0];
        outcome = std::string(prefix) +
            (value.isString() ? value.getString(rt).utf8(rt)
                              : value.getObject(rt).getProperty(rt, "message").getString(rt).utf8(rt));
        return Value::undefined();
      };
    };
    auto onResolve = Function::createFromHostFunction(rt, PropNameID::forAscii(rt, "resolved"), 1, record("resolved: "));
    auto onReject = Function::createFromHostFunction(rt, PropNameID::forAscii(rt, "rejected"), 1, record("rejected: "));
    promise.getPropertyAsFunction(rt, "then").callWithThis(rt, promise, onResolve, onReject);
    runMainLoopBriefly();
    invoker->run(rt);
    rt.drainMicrotasks();
    return outcome;
  }

  std::unique_ptr<facebook::hermes::HermesRuntime> runtime{
      facebook::hermes::makeHermesRuntime(::hermes::vm::RuntimeConfig::Builder().withMicrotaskQueue(true).build())};
  std::shared_ptr<QueuedInvoker> invoker{std::make_shared<QueuedInvoker>()};
  std::shared_ptr<TvState> tv{std::make_shared<TvState>(false, std::nullopt)};
  std::shared_ptr<PreviewModule> module;
};

} // namespace

TEST_CASE_FIXTURE(Preview, "RNTizenSmartHubPreview is available once the shell gives it a publisher") {
  auto &rt = *runtime;
  CHECK_FALSE(call(rt, *module, "isAvailable").getBool());

  tv->setPreviewPublisher([](const std::string &) { return 1; });
  tv->setLaunchedFromTile(true);

  CHECK(call(rt, *module, "isAvailable").getBool());
  CHECK(call(rt, *module, "launchedFromTile").getBool());
  CHECK(call(rt, *module, "setSource").isUndefined());
}

TEST_CASE_FIXTURE(Preview, "publish resolves with the channel that took the preview") {
  std::string published;
  tv->setPreviewPublisher([&published](const std::string &json) {
    published = json;
    return 1;
  });

  const auto outcome = publish(R"({"sections":[]})");

  CHECK(outcome == "resolved: ContentDBAPI");
  CHECK(published == R"({"sections":[]})");
}

TEST_CASE_FIXTURE(Preview, "publish rejects when no channel took the preview or there is no Smart Hub") {
  tv->setPreviewPublisher([](const std::string &) { return 0; });
  const auto refused = publish("{}");
  tv->setPreviewPublisher(nullptr);
  const auto missing = publish("{}");

  CHECK(refused == "rejected: Smart Hub preview: the TV took the preview through no channel");
  CHECK(missing == "rejected: Smart Hub preview needs a Samsung TV: this device has no Smart Hub");
}

TEST_CASE_FIXTURE(Preview, "publish refuses what is not a JSON string") {
  auto &rt = *runtime;

  CHECK_THROWS_AS(call(rt, *module, "publish", Value(3)), facebook::jsi::JSError);
}

} // namespace rntizen
