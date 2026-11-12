#pragma once

#include "Context.h"
#include "ExpoRuntime.h"

#include "threading/MainLoop.test.h"

#include <Ecore.h>
#include <hermes/hermes.h>
#include <jsi/jsi.h>

#include <chrono>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

namespace rntizen::expo::testing {

/// The app's data, cache and resource directories, in a directory of their own
/// that the simulator's app_common shim answers for: set before any test runs,
/// so appDirectories() reads them the first time it is asked.
inline const std::filesystem::path &testAppRoot() {
  static const std::filesystem::path root = [] {
    auto pattern = (std::filesystem::temp_directory_path() / "rn-tizen-expo-tests-XXXXXX").string();
    if (mkdtemp(pattern.data()) == nullptr) throw std::runtime_error("cannot create the test app directory");
    const std::filesystem::path made(pattern);
    for (const auto *name : {"data", "cache", "res"}) std::filesystem::create_directories(made / name);
    setenv("RN_TIZEN_SIMULATOR_DATA_DIR", (made / "data").c_str(), 1);
    setenv("RN_TIZEN_SIMULATOR_CACHE_DIR", (made / "cache").c_str(), 1);
    setenv("RN_TIZEN_SIMULATOR_RESOURCE_DIR", (made / "res").c_str(), 1);
    return made;
  }();
  return root;
}

inline const bool testAppRootReady = !testAppRoot().empty();

/// A CallInvoker that queues work for the thread that drains it, as React
/// Native's JavaScript thread runs what native code hands it.
class QueuedInvoker final : public facebook::react::CallInvoker {
 public:
  void invokeAsync(facebook::react::CallFunc &&task) noexcept override {
    std::lock_guard lock(mutex_);
    tasks_.push_back(std::move(task));
  }

  void invokeSync(facebook::react::CallFunc &&task) override {
    std::lock_guard lock(mutex_);
    tasks_.push_back(std::move(task));
  }

  bool drain(facebook::jsi::Runtime &runtime) {
    std::deque<facebook::react::CallFunc> ready;
    {
      std::lock_guard lock(mutex_);
      ready.swap(tasks_);
    }
    for (auto &task : ready) task(runtime);
    return !ready.empty();
  }

 private:
  std::mutex mutex_;
  std::deque<facebook::react::CallFunc> tasks_;
};

/// Hermes with `globalThis.expo` installed over a Context of its own. The test's
/// thread is the JavaScript thread and the EFL main loop at once: `settle` and
/// `pumpUntil` run what the background pool, the main loop and native callbacks
/// queued until the awaited result arrives.
class ExpoTestRuntime final {
 public:
  explicit ExpoTestRuntime(ModuleContext host = {})
      : context_(std::make_shared<Context>(invoker_, std::move(host))) {
    installExpoGlobals(*runtime_, context_);
  }

  ExpoTestRuntime(const ExpoTestRuntime &) = delete;
  ExpoTestRuntime &operator=(const ExpoTestRuntime &) = delete;

  facebook::jsi::Runtime &runtime() {
    return *runtime_;
  }

  const std::shared_ptr<Context> &context() const {
    return context_;
  }

  facebook::jsi::Value eval(const std::string &source) {
    auto value = runtime_->evaluateJavaScript(std::make_shared<facebook::jsi::StringBuffer>(source), "test.js");
    runtime_->drainMicrotasks();
    return value;
  }

  /// `JSON.stringify` of what `source` evaluates to.
  std::string json(const std::string &source) {
    return eval("JSON.stringify(" + source + ")").asString(*runtime_).utf8(*runtime_);
  }

  /// Evaluates `source`, then runs the queued work until the promise it gives settles; returns
  /// `JSON.stringify` of the value, or throws std::runtime_error with the rejection's
  /// `code: message`.
  std::string settle(const std::string &source) {
    eval(
        "globalThis.__settled = undefined; Promise.resolve(" + source +
        ").then(value => { globalThis.__settled = { value: JSON.stringify(value) ?? 'undefined' }; },"
        " error => { globalThis.__settled = { error: (error && error.code ? error.code + ': ' : '') + String(error && error.message !== undefined ? error.message : error) }; });");
    if (!pumpUntil([this] { return eval("globalThis.__settled !== undefined").getBool(); })) {
      throw std::runtime_error("the promise did not settle");
    }
    auto settled = eval("globalThis.__settled").asObject(*runtime_);
    if (auto error = settled.getProperty(*runtime_, "error"); error.isString()) {
      throw std::runtime_error(error.asString(*runtime_).utf8(*runtime_));
    }
    return settled.getProperty(*runtime_, "value").asString(*runtime_).utf8(*runtime_);
  }

  /// Runs the JavaScript queue, microtasks and the main loop until `done()` or five seconds pass.
  bool pumpUntil(const std::function<bool()> &done) {
    const auto giveUp = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < giveUp) {
      pump();
      if (done()) return true;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return false;
  }

  /// One round of the JavaScript queue, microtasks and the main loop.
  void pump() {
    invoker_->drain(*runtime_);
    runtime_->drainMicrotasks();
    ecore_main_loop_iterate();
  }

 private:
  rntizen::testing::MainLoopScope mainLoop_;
  std::shared_ptr<QueuedInvoker> invoker_ = std::make_shared<QueuedInvoker>();
  std::shared_ptr<Context> context_;
  std::unique_ptr<facebook::jsi::Runtime> runtime_ =
      facebook::hermes::makeHermesRuntime(::hermes::vm::RuntimeConfig::Builder().withMicrotaskQueue(true).build());
};

} // namespace rntizen::expo::testing
