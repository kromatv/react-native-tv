#include "PreviewModule.h"

#include "AppDataDirectory.test.h"
#include "TvState.h"

#include <doctest/doctest.h>

#include <Ecore.h>
#include <hermes/hermes.h>

#include <filesystem>
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
    module = makeModule(tv);
  }
  ~Preview() {
    ecore_shutdown();
  }
  Preview(const Preview &) = delete;
  Preview &operator=(const Preview &) = delete;

  std::shared_ptr<PreviewModule> makeModule(std::shared_ptr<TvState> state, std::string platformVersion = "") const {
    ModuleContext context{};
    context.tv = std::move(state);
    context.device.platformVersion = std::move(platformVersion);
    return std::make_shared<PreviewModule>(invoker, context);
  }

  void startPublish(const char *json) {
    auto &rt = *runtime;
    auto promise = call(rt, *module, "publish", String::createFromAscii(rt, json)).asObject(rt);
    auto record = [this](const char *prefix) {
      return [this, prefix](Runtime &rt, const Value &, const Value *arguments, size_t) {
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
  }

  void runJavaScript() {
    invoker->run(*runtime);
    runtime->drainMicrotasks();
  }

  std::string publish(const char *json) {
    startPublish(json);
    runMainLoopBriefly();
    runJavaScript();
    return outcome;
  }

  const testing::AppDataDirectory data;
  std::unique_ptr<facebook::hermes::HermesRuntime> runtime{
      facebook::hermes::makeHermesRuntime(::hermes::vm::RuntimeConfig::Builder().withMicrotaskQueue(true).build())};
  std::shared_ptr<QueuedInvoker> invoker{std::make_shared<QueuedInvoker>()};
  std::shared_ptr<TvState> tv{std::make_shared<TvState>(false, std::nullopt)};
  std::shared_ptr<PreviewModule> module;
  std::string outcome{"pending"};
};

} // namespace

TEST_CASE_FIXTURE(Preview, "RNTizenSmartHubPreview is available once the shell gives it a publisher") {
  auto &rt = *runtime;
  CHECK_FALSE(call(rt, *module, "isAvailable").getBool());
  CHECK(call(rt, *module, "unavailableReason").getString(rt).utf8(rt).starts_with("no personal Smart Hub preview"));
  CHECK_FALSE(call(rt, *module, "launchedFromTile").getBool());

  tv->setPreviewPublisher([](const std::string &) { return 1; });
  tv->setLaunchedFromTile(true);

  CHECK(call(rt, *module, "isAvailable").getBool());
  CHECK(call(rt, *module, "unavailableReason").isNull());
  CHECK(call(rt, *module, "launchedFromTile").getBool());
}

TEST_CASE_FIXTURE(Preview, "RNTizenSmartHubPreview names the Tizen the preview needs on an older TV") {
  auto &rt = *runtime;
  tv->setPreviewPublisher([](const std::string &) { return 1; });
  module = makeModule(tv, "2.3");

  CHECK_FALSE(call(rt, *module, "isAvailable").getBool());
  CHECK(call(rt, *module, "unavailableReason").getString(rt).utf8(rt) == "Smart Hub Preview needs Tizen 2.4, and this TV runs 2.3");
}

TEST_CASE_FIXTURE(Preview, "RNTizenSmartHubPreview without the TV's state is unavailable and rejects a publish") {
  auto &rt = *runtime;
  module = makeModule(nullptr);

  CHECK_FALSE(call(rt, *module, "isAvailable").getBool());
  CHECK_FALSE(call(rt, *module, "launchedFromTile").getBool());
  CHECK(publish("{}") == "rejected: Smart Hub preview needs a Samsung TV: this device has no Smart Hub");
}

TEST_CASE_FIXTURE(Preview, "setSource keeps where the preview service fetches tiles, and forgets it without one") {
  auto &rt = *runtime;
  const auto source = R"({"url":"https://example.tv/preview.json"})";

  CHECK(call(rt, *module, "setSource", String::createFromAscii(rt, source)).isUndefined());
  CHECK(data.read("preview-source.json") == source);
  CHECK(call(rt, *module, "setSource").isUndefined());
  CHECK_FALSE(std::filesystem::exists(data.path() / "preview-source.json"));
  call(rt, *module, "setSource", String::createFromAscii(rt, source));
  call(rt, *module, "setSource", Value::null());
  CHECK_FALSE(std::filesystem::exists(data.path() / "preview-source.json"));
}

TEST_CASE_FIXTURE(Preview, "publish resolves with the channel that took the preview and keeps it") {
  std::string published;
  tv->setPreviewPublisher([&published](const std::string &json) {
    published = json;
    return 1;
  });

  const auto outcome = publish(R"({"sections":[]})");

  CHECK(outcome == "resolved: ContentDBAPI");
  CHECK(published == R"({"sections":[]})");
  CHECK(data.read("preview.json") == R"({"sections":[]})");
}

TEST_CASE_FIXTURE(Preview, "publish resolves with the message port when it took the preview") {
  tv->setPreviewPublisher([](const std::string &) { return 2; });

  CHECK(publish("{}") == "resolved: message port");
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
  CHECK_THROWS_AS(call(rt, *module, "publish"), facebook::jsi::JSError);
}

TEST_CASE_FIXTURE(Preview, "publish settles nothing once the module is gone before the main loop hands it over") {
  bool handedOver = false;
  tv->setPreviewPublisher([&handedOver](const std::string &) {
    handedOver = true;
    return 1;
  });

  startPublish("{}");
  module.reset();
  runMainLoopBriefly();
  runJavaScript();

  CHECK(handedOver);
  CHECK(invoker->tasks.empty());
  CHECK(outcome == "pending");
}

TEST_CASE_FIXTURE(Preview, "publish settles nothing once the module is gone before JavaScript hears back") {
  tv->setPreviewPublisher([](const std::string &) { return 1; });

  startPublish("{}");
  runMainLoopBriefly();
  module.reset();
  runJavaScript();

  CHECK(outcome == "pending");
}

} // namespace rntizen
