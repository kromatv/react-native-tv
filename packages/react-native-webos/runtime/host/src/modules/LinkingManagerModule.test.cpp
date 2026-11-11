#include "LinkingManagerModule.h"

#include "LaunchUrl.h"
#include "TurboModuleTest.test.h"

#include <doctest/doctest.h>

#include <memory>

namespace rntizen {

namespace {

using facebook::jsi::String;
using testing::call;
using testing::settled;

std::shared_ptr<LinkingManagerModule> linking(std::shared_ptr<const LaunchUrl> launchUrl) {
  ModuleContext context{};
  context.launchUrl = std::move(launchUrl);
  return std::make_shared<LinkingManagerModule>(std::make_shared<testing::QueuedInvoker>(), context);
}

} // namespace

TEST_CASE("getInitialURL answers the URI of the app control that launched the app, once the host set it") {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  auto launchUrl = std::make_shared<LaunchUrl>();
  const auto module = linking(launchUrl);

  CHECK(settled(rt, call(rt, *module, "getInitialURL")) == "resolved: null");
  launchUrl->set("kroma://title/42");
  CHECK(launchUrl->get() == "kroma://title/42");
  CHECK(settled(rt, call(rt, *module, "getInitialURL")) == R"(resolved: "kroma://title/42")");
}

TEST_CASE("without a launch URL getInitialURL answers null, and nothing else can be opened") {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  const auto module = linking(nullptr);
  const auto url = String::createFromUtf8(rt, "https://example.com");

  CHECK(settled(rt, call(rt, *module, "getInitialURL")) == "resolved: null");
  CHECK(settled(rt, call(rt, *module, "canOpenURL", url)) == "resolved: false");
  CHECK(settled(rt, call(rt, *module, "openURL", url)) ==
        "rejected: Cannot open https://example.com: Linking.openURL is not supported on Tizen yet");
  CHECK(settled(rt, call(rt, *module, "openSettings")) == "rejected: Linking.openSettings is not supported on Tizen");
  CHECK(call(rt, *module, "addListener", String::createFromUtf8(rt, "url")).isUndefined());
  CHECK(call(rt, *module, "removeListeners", 1).isUndefined());
}

} // namespace rntizen
