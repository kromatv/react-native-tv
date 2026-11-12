#include "Inspector.h"

#include <doctest/doctest.h>

#include <Ecore.h>
#include <jsinspector-modern/InspectorInterfaces.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace rntizen {

namespace {

using facebook::react::jsinspector_modern::getInspectorInstance;
using facebook::react::jsinspector_modern::InspectorPageDescription;
using facebook::react::jsinspector_modern::IRemoteConnection;

class RecordingFrontend final : public IRemoteConnection {
 public:
  explicit RecordingFrontend(std::vector<std::string> &messages) : messages_(messages) {}
  void onMessage(std::string message) override {
    messages_.push_back(std::move(message));
  }
  void onDisconnect() override {}

 private:
  std::vector<std::string> &messages_;
};

const DeviceProfile kDevice{
    .modelName = "QE55Q80",
    .platformVersion = "9.0",
    .buildString = "T-PTMAKUC",
    .screenWidth = 1920,
    .screenHeight = 1080,
};

int bridgelessPageCount() {
  int count = 0;
  for (const auto &page : getInspectorInstance().getPages()) {
    if (page.description == "React Native Bridgeless") count += 1;
  }
  return count;
}

std::optional<InspectorPageDescription> bridgelessPage() {
  std::optional<InspectorPageDescription> found;
  for (const auto &page : getInspectorInstance().getPages()) {
    if (!found.has_value() && page.description == "React Native Bridgeless") found.emplace(page);
  }
  return found;
}

} // namespace

TEST_CASE("The inspector lists the app as a page DevTools can reload, until it goes") {
  ecore_init();
  const auto server = devServerOf("http://127.0.0.1:9/index.bundle?app=org.example.app");
  {
    Inspector inspector(*server, kDevice, {.reload = [] {}, .showPausedMessage = [](const auto &) {}});

    CHECK(bridgelessPageCount() == 1);
    const auto page = bridgelessPage();
    REQUIRE(page.has_value());
    CHECK(page->vm.empty());
    CHECK(page->capabilities.nativePageReloads);
  }
  CHECK(bridgelessPageCount() == 0);
}

TEST_CASE("The inspector names the app, the TV and the platform to DevTools") {
  ecore_init();
  const auto server = devServerOf("http://127.0.0.1:9/index.bundle?app=org.example.app");
  Inspector inspector(*server, kDevice, {.reload = [] {}, .showPausedMessage = [](const auto &) {}});

  const auto metadata = inspector.getMetadata();

  CHECK(metadata.appIdentifier == "org.example.app");
  CHECK(metadata.appDisplayName == "org.example.app");
  CHECK(metadata.deviceName == "QE55Q80");
  CHECK(metadata.platform == "tizen");
  CHECK(metadata.integrationName == "Tizen Bridgeless (ReactHost)");
  REQUIRE(metadata.reactNativeVersion.has_value());
  CHECK(metadata.reactNativeVersion->starts_with("0."));
}

TEST_CASE("A DevTools session attaches to the inspector's target and is answered") {
  ecore_init();
  const auto server = devServerOf("http://127.0.0.1:9/index.bundle?app=org.example.app");
  Inspector inspector(*server, kDevice, {.reload = [] {}, .showPausedMessage = [](const auto &) {}});
  std::vector<std::string> answers;

  auto session = inspector.target().connect(std::make_unique<RecordingFrontend>(answers));
  session->sendMessage(R"({"id":1,"method":"Page.enable"})");
  for (int turn = 0; turn < 100 && answers.empty(); ++turn) ecore_main_loop_iterate();

  REQUIRE_FALSE(answers.empty());
  CHECK(answers.front().find(R"("id":1)") != std::string::npos);
}

TEST_CASE("DevTools' reload and paused message reach the host, and resuming needs no runtime") {
  ecore_init();
  const auto server = devServerOf("http://127.0.0.1:9/index.bundle?app=org.example.app");
  int reloads = 0;
  std::vector<std::optional<std::string>> messages;
  Inspector inspector(
      *server,
      kDevice,
      {.reload = [&reloads] { reloads += 1; },
       .showPausedMessage = [&messages](const auto &message) { messages.push_back(message); }});

  inspector.onReload({.ignoreCache = std::nullopt, .scriptToEvaluateOnLoad = std::nullopt});
  inspector.onSetPausedInDebuggerMessage({.message = "Paused in debugger"});
  inspector.onSetPausedInDebuggerMessage({.message = std::nullopt});
  inspector.resume();

  CHECK(reloads == 1);
  REQUIRE(messages.size() == 2);
  CHECK(messages[0] == "Paused in debugger");
  CHECK_FALSE(messages[1].has_value());
}

} // namespace rntizen
