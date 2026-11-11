#pragma once

#include "../fabric/Components.test.h"
#include "../fabric/SurfacePresenter.h"
#include "../input/KeyboardFocus.h"
#include "../intl/IcuData.h"
#include "../platform/MainLoop.h"
#include "TextInputView.h"

#include <doctest/doctest.h>
#include <folly/json.h>
#include <hermes/hermes.h>
#include <react/renderer/runtimescheduler/RuntimeScheduler.h>
#include <react/utils/ContextContainer.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace rnwebos::testing {

/// One `AndroidTextInput` on a Fabric surface that the host's SurfacePresenter
/// mounts in a scene with no window. The test is the JavaScript side: it
/// commits the field and its props, sends its commands, and gets every event
/// the field sends, as `type {payload}` with the payload's keys sorted. Keys
/// and text reach the field as SDL events through the scene's KeyboardFocus.
class TextInputSurface final {
 public:
  static constexpr facebook::react::Tag kFieldTag = 2;

  /// Mounts the field with `props`, a JavaScript object literal.
  explicit TextInputSurface(const std::string &props) {
    static const bool hasIcuData = (useIcuData(RN_WEBOS_TEST_ICU_DATA_DIR), true);
    REQUIRE(hasIcuData);
    evaluate(kJavaScriptSide);
    presenter_.startSurface("field", {.width = 1920, .height = 1080});
    evaluate("mount(" + props + ");");
    settle();
  }

  ~TextInputSurface() {
    settle();
    presenter_.stopSurface();
    settle();
  }

  TextInputSurface(const TextInputSurface &) = delete;
  TextInputSurface &operator=(const TextInputSurface &) = delete;

  TextInputView &field() {
    auto *view = dynamic_cast<TextInputView *>(presenter_.mountingManager().findView(kFieldTag));
    REQUIRE(view != nullptr);
    return *view;
  }

  KeyboardFocus &keyboard() {
    return keyboard_;
  }

  /// Commits the field again with `props` merged into its own, as a render does.
  void setProps(const std::string &props) {
    evaluate("update(" + props + ");");
    settle();
  }

  void command(const std::string &name, const std::string &arguments = "[]") {
    evaluate("nativeFabricUIManager.dispatchCommand(field, '" + name + "', " + arguments + ");");
    settle();
  }

  /// A key going down and up. True when the field used the press.
  bool press(SDL_Scancode scancode, SDL_Keymod modifiers = SDL_KMOD_NONE, SDL_Keycode keycode = SDLK_UNKNOWN) {
    SDL_KeyboardEvent key{};
    key.scancode = scancode;
    key.key = keycode;
    key.mod = modifiers;
    const bool used = keyboard_.handleKey(key, true);
    keyboard_.handleKey(key, false);
    settle();
    return used;
  }

  void commit(const char *text) {
    SDL_Event event{};
    event.type = SDL_EVENT_TEXT_INPUT;
    event.text.text = text;
    keyboard_.handleTextEvent(event);
    settle();
  }

  void compose(const char *text) {
    SDL_Event event{};
    event.type = SDL_EVENT_TEXT_EDITING;
    event.edit.text = text;
    event.edit.start = -1;
    event.edit.length = -1;
    keyboard_.handleTextEvent(event);
  }

  void showKeyboard() {
    SDL_Event event{};
    event.type = SDL_EVENT_SCREEN_KEYBOARD_SHOWN;
    keyboard_.handleTextEvent(event);
  }

  /// The events JavaScript got since the last call, oldest first.
  std::vector<std::string> takeEvents() {
    settle();
    std::vector<std::string> events;
    for (const auto &event : folly::parseJson(evaluate("return JSON.stringify(events.splice(0));"))) {
      events.push_back(event.asString());
    }
    return events;
  }

  /// Runs what the last step left for the main loop: mounts, commands and the
  /// commits state updates make.
  void settle() {
    for (int turn = 0; turn < kSettleTurns; ++turn) MainLoop::get().iterate();
  }

 private:
  static constexpr int kSettleTurns = 4;

  static constexpr const char *kJavaScriptSide = R"(
    globalThis.RN$AppRegistry = { runApplication() {}, setSurfaceProps() {} };
    globalThis.RN$stopSurface = function () {};
    globalThis.events = [];
    function stable(value) {
      if (value === null || typeof value !== 'object') return JSON.stringify(value);
      const fields = Object.keys(value).sort().map(function (key) { return key + ':' + stable(value[key]); });
      return '{' + fields.join(',') + '}';
    }
    nativeFabricUIManager.registerEventHandler(function (instanceHandle, type, payload) {
      delete payload.timeStamp;
      events.push(type + ' ' + stable(payload));
    });
    function commit() {
      const children = nativeFabricUIManager.createChildSet(1);
      nativeFabricUIManager.appendChildToSet(children, field);
      nativeFabricUIManager.completeRoot(1, children);
    }
    globalThis.mount = function (props) {
      globalThis.field = nativeFabricUIManager.createNode(2, 'AndroidTextInput', 1, props, {});
      commit();
    };
    globalThis.update = function (props) {
      field = nativeFabricUIManager.cloneNodeWithNewProps(field, props);
      commit();
    };
  )";

  facebook::react::RuntimeExecutor executor() {
    return [this](std::function<void(facebook::jsi::Runtime &)> &&callback) { callback(*runtime_); };
  }

  std::shared_ptr<const facebook::react::ContextContainer> context() const {
    auto context = std::make_shared<facebook::react::ContextContainer>();
    context->insert(
        facebook::react::RuntimeSchedulerKey, std::weak_ptr<facebook::react::RuntimeScheduler>(runtimeScheduler_));
    context->insert(
        facebook::react::TextMeasurer::contextKey,
        std::shared_ptr<const facebook::react::TextMeasurer>(std::make_shared<RecordingMeasurer>()));
    return context;
  }

  /// Runs `source` as a task of the runtime scheduler, as React's own code
  /// runs: Fabric hands commands over only at the end of such a task.
  std::string evaluate(const std::string &source) {
    std::string result;
    runtimeScheduler_->scheduleWork([&result, &source](facebook::jsi::Runtime &runtime) {
      const auto script = "(function () { try {" + source + "} catch (error) { return 'threw ' + error; } })()";
      const auto value = runtime.evaluateJavaScript(std::make_shared<facebook::jsi::StringBuffer>(script), "test.js");
      if (value.isString()) result = value.getString(runtime).utf8(runtime);
    });
    REQUIRE_MESSAGE(!result.starts_with("threw "), result);
    return result;
  }

  const std::unique_ptr<facebook::hermes::HermesRuntime> runtime_{facebook::hermes::makeHermesRuntime()};
  const std::shared_ptr<facebook::react::RuntimeScheduler> runtimeScheduler_{
      std::make_shared<facebook::react::RuntimeScheduler>(executor())};
  Scene scene_{PixelSize{.width = 1920, .height = 1080}};
  KeyboardFocus keyboard_{scene_};
  SurfacePresenter presenter_{
      scene_,
      {.contextContainer = context(),
       .runtimeScheduler = runtimeScheduler_,
       .runtimeExecutor = executor(),
       .bridgelessBindingsExecutor = executor()}};
};

} // namespace rnwebos::testing
