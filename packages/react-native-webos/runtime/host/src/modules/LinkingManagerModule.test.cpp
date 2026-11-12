#include "LinkingManagerModule.h"

#include "../platform/FakeLuna.test.h"
#include "../threading/MainLoop.test.h"
#include "LaunchState.h"
#include "TurboModuleTest.test.h"

#include <doctest/doctest.h>

#include <memory>

namespace rnwebos {

namespace {

using facebook::jsi::String;
using testing::call;
using testing::settled;

constexpr const char *kLaunch = "luna://com.webos.applicationManager/launch";

struct Linking {
  explicit Linking(LunaBus *bus = nullptr, std::shared_ptr<const LaunchState> launch = nullptr) {
    ModuleContext context{};
    context.launch = std::move(launch);
    context.luna = bus;
    module = std::make_shared<LinkingManagerModule>(invoker, context);
  }

  /// How the promise `method(url)` gives settles, once the TV answered and JavaScript ran.
  std::string settle(facebook::jsi::Runtime &rt, const char *method, const std::string &url) const {
    const auto promise = call(rt, *module, method, String::createFromUtf8(rt, url));
    testing::FakeLuna::deliver();
    invoker->run(rt);
    return settled(rt, promise);
  }

  std::shared_ptr<testing::QueuedInvoker> invoker = std::make_shared<testing::QueuedInvoker>();
  std::shared_ptr<LinkingManagerModule> module;
};

} // namespace

TEST_CASE("getInitialURL answers the deep link the TV launched the app with, once the host set it") {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  auto launch = std::make_shared<LaunchState>();
  const Linking linking(nullptr, launch);

  CHECK(settled(rt, call(rt, *linking.module, "getInitialURL")) == "resolved: null");
  launch->setInitialUrl("kroma://title/42");
  CHECK(settled(rt, call(rt, *linking.module, "getInitialURL")) == R"(resolved: "kroma://title/42")");
}

TEST_CASE("openURL hands an http or https URL to the TV's browser through the app manager") {
  const testing::MainLoopScope mainLoop;
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  testing::FakeLuna luna;
  luna.answer(kLaunch, R"({"returnValue":true})");
  LunaBus bus(luna.transport());
  const Linking linking(&bus);

  CHECK(linking.settle(rt, "canOpenURL", "https://example.com/a?b=c") == "resolved: true");
  CHECK(linking.settle(rt, "openURL", "https://example.com/a?b=c") == "resolved: null");
  REQUIRE(luna.calls().size() == 1);
  CHECK(luna.calls()[0].uri == kLaunch);
  CHECK(luna.calls()[0].payload ==
        folly::dynamic::object("id", "com.webos.app.browser")(
            "params", folly::dynamic::object("target", "https://example.com/a?b=c")));
}

TEST_CASE("openURL rejects with the app manager's reason when it refuses") {
  const testing::MainLoopScope mainLoop;
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  testing::FakeLuna luna;
  luna.answer(kLaunch, R"({"returnValue":false,"errorCode":-101,"errorText":"The app was not found."})");
  LunaBus bus(luna.transport());
  const Linking linking(&bus);

  CHECK(linking.settle(rt, "openURL", "http://example.com") ==
        "rejected: Cannot open http://example.com: The app was not found.");
}

TEST_CASE("only web URLs open, and none without the TV's bus") {
  const testing::MainLoopScope mainLoop;
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  testing::FakeLuna luna;
  LunaBus bus(luna.transport());
  const Linking onTv(&bus);
  const Linking alone;

  CHECK(onTv.settle(rt, "canOpenURL", "netflix://title/1") == "resolved: false");
  CHECK(onTv.settle(rt, "openURL", "netflix://title/1") ==
        "rejected: Cannot open netflix://title/1: webOS opens http and https URLs only, in its browser");
  CHECK(alone.settle(rt, "canOpenURL", "https://example.com") == "resolved: false");
  CHECK(alone.settle(rt, "openURL", "https://example.com") ==
        "rejected: Cannot open https://example.com: the TV's app manager cannot be reached");
  CHECK(luna.calls().empty());
  CHECK(settled(rt, call(rt, *alone.module, "openSettings")) == "rejected: Linking.openSettings is not supported on webOS");
  CHECK(call(rt, *alone.module, "addListener", String::createFromUtf8(rt, "url")).isUndefined());
  CHECK(call(rt, *alone.module, "removeListeners", 1).isUndefined());
}

} // namespace rnwebos
