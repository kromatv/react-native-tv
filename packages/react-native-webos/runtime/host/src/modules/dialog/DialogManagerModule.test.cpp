#include "DialogManagerModule.h"

#include "../TurboModuleTest.test.h"
#include "AlertDialog.h"

#include <doctest/doctest.h>

#include <memory>
#include <string>
#include <vector>

namespace rnwebos {

namespace {

using testing::call;
using testing::evaluate;

/// A JavaScript function that keeps the action and the button key `onAction` gives it.
facebook::jsi::Function actionRecorder(facebook::jsi::Runtime &runtime, std::shared_ptr<std::vector<std::string>> actions) {
  return facebook::jsi::Function::createFromHostFunction(
      runtime,
      facebook::jsi::PropNameID::forAscii(runtime, "onAction"),
      2,
      [actions](facebook::jsi::Runtime &rt, const facebook::jsi::Value &, const facebook::jsi::Value *arguments, size_t count) {
        const auto action = arguments[0].asString(rt).utf8(rt);
        const bool hasKey = count > 1 && arguments[1].isNumber();
        actions->push_back(action + ":" + (hasKey ? std::to_string(static_cast<int>(arguments[1].getNumber())) : "none"));
        return facebook::jsi::Value::undefined();
      });
}

struct Dialogs {
  std::unique_ptr<facebook::jsi::Runtime> runtime{testing::makeRuntime()};
  std::shared_ptr<testing::QueuedInvoker> invoker{std::make_shared<testing::QueuedInvoker>()};
  std::vector<AlertRequest> shown;
  std::shared_ptr<std::vector<std::string>> errors{std::make_shared<std::vector<std::string>>()};
  std::shared_ptr<std::vector<std::string>> actions{std::make_shared<std::vector<std::string>>()};
  DialogManagerModule module{invoker, contextShowingAlerts()};

  ModuleContext contextShowingAlerts() {
    ModuleContext context{};
    context.showAlert = [this](AlertRequest request) { shown.push_back(std::move(request)); };
    return context;
  }

  void showAlert(const std::string &config) {
    auto &rt = *runtime;
    call(rt, module, "showAlert", evaluate(rt, config), testing::recorder(rt, errors), actionRecorder(rt, actions));
  }
};

} // namespace

TEST_CASE_FIXTURE(Dialogs, "DialogManagerAndroid hands the host an alert's title, message and buttons, the neutral one first") {
  showAlert(
      "({title: 'Delete?', message: 'Gone for good', buttonPositive: 'Delete', buttonNegative: 'Cancel',"
      " buttonNeutral: 'Later', cancelable: true})");

  REQUIRE(shown.size() == 1);
  const auto &alert = shown[0];
  CHECK(alert.title == "Delete?");
  CHECK(alert.message == "Gone for good");
  CHECK(alert.cancelable);
  REQUIRE(alert.buttons.size() == 3);
  CHECK(alert.buttons[0].text == "Later");
  CHECK(alert.buttons[0].key == -3);
  CHECK(alert.buttons[1].text == "Cancel");
  CHECK(alert.buttons[1].key == -2);
  CHECK(alert.buttons[2].text == "Delete");
  CHECK(alert.buttons[2].key == -1);
}

TEST_CASE_FIXTURE(Dialogs, "DialogManagerAndroid shows an alert with what it has, not cancelable unless asked") {
  showAlert("({buttonPositive: 'OK'})");

  REQUIRE(shown.size() == 1);
  CHECK(shown[0].title.empty());
  CHECK(shown[0].message.empty());
  CHECK_FALSE(shown[0].cancelable);
  REQUIRE(shown[0].buttons.size() == 1);
  CHECK(shown[0].buttons[0].key == -1);
}

TEST_CASE_FIXTURE(Dialogs, "DialogManagerAndroid tells JavaScript which button closed an alert, or that it was dismissed") {
  showAlert("({title: 'First', buttonPositive: 'OK'})");
  showAlert("({title: 'Second', buttonPositive: 'OK', cancelable: true})");
  REQUIRE(shown.size() == 2);

  shown[0].onClose(-1);
  shown[1].onClose(std::nullopt);
  invoker->run(*runtime);

  CHECK(*actions == std::vector<std::string>{"buttonClicked:-1", "dismissed:none"});
  CHECK(errors->empty());
}

TEST_CASE_FIXTURE(Dialogs, "DialogManagerAndroid refuses a list of items") {
  showAlert("({title: 'Pick one', items: ['a', 'b']})");
  invoker->run(*runtime);

  CHECK(shown.empty());
  CHECK(*errors == std::vector<std::string>{"\"Alert items are not supported on webOS\""});
}

TEST_CASE_FIXTURE(Dialogs, "DialogManagerAndroid names its actions and button keys as Android does") {
  auto &rt = *runtime;

  const auto constants = facebook::jsi::dynamicFromValue(rt, call(rt, module, "getConstants"));

  CHECK(
      constants ==
      folly::dynamic::object("buttonClicked", "buttonClicked")("dismissed", "dismissed")("buttonPositive", -1)(
          "buttonNegative", -2)("buttonNeutral", -3));
}

} // namespace rnwebos
