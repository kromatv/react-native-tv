#include "InstanceGate.h"

#include "../timers/ThreadTimerRegistry.h"
#include "TaskQueueThread.h"

#include <cxxreact/JSBigString.h>
#include <doctest/doctest.h>
#include <jsi/jsi.h>
#include <react/runtime/hermes/HermesInstance.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace rntizen {

namespace {

using facebook::jsi::Function;
using facebook::jsi::PropNameID;
using facebook::jsi::Runtime;
using facebook::jsi::Value;
using facebook::react::JsErrorHandler;
using facebook::react::ReactInstance;
using facebook::react::TimerManager;

constexpr const char *kProbe = "RN$registerCallableModule('Probe', () => ({ say: (word) => heard(word) }));";

class ProbeInstance final {
 public:
  ProbeInstance() {
    auto registry = std::make_unique<ThreadTimerRegistry>();
    auto *registryHandle = registry.get();
    timers_ = std::make_shared<TimerManager>(std::move(registry));
    registryHandle->setTimerManager(timers_);
    instance_ = std::make_unique<ReactInstance>(
        facebook::react::HermesInstance::createJSRuntime(nullptr, jsThread_, false),
        jsThread_,
        timers_,
        [this](Runtime &, const JsErrorHandler::ProcessedError &error) { heard_.push_back("error " + error.message); },
        nullptr);
    timers_->setRuntimeExecutor(instance_->getBufferedRuntimeExecutor());
    instance_->initializeRuntime({.isProfiling = false}, [this](Runtime &runtime) {
      auto heard = [this](Runtime &js, const Value &, const Value *arguments, std::size_t) {
        heard_.push_back(arguments[0].asString(js).utf8(js));
        return Value::undefined();
      };
      auto function = Function::createFromHostFunction(runtime, PropNameID::forAscii(runtime, "heard"), 1, heard);
      runtime.global().setProperty(runtime, "heard", std::move(function));
    });
    instance_->loadScript(std::make_unique<facebook::react::JSBigStdString>(kProbe), "probe.js");
  }

  ~ProbeInstance() {
    jsThread_->runOnQueueSync([this] {
      timers_.reset();
      instance_.reset();
    });
    jsThread_->quitSynchronous();
  }

  ProbeInstance(const ProbeInstance &) = delete;
  ProbeInstance &operator=(const ProbeInstance &) = delete;

  ReactInstance &instance() const {
    return *instance_;
  }

  std::vector<std::string> heard() const {
    std::vector<std::string> words;
    jsThread_->runOnQueueSync([this, &words] { words = heard_; });
    return words;
  }

 private:
  const std::shared_ptr<TaskQueueThread> jsThread_ = std::make_shared<TaskQueueThread>("rn-js-gate-test");
  std::shared_ptr<TimerManager> timers_;
  std::unique_ptr<ReactInstance> instance_;
  std::vector<std::string> heard_;
};

} // namespace

TEST_CASE("InstanceGate passes calls to JavaScript until it is closed, then drops them") {
  const ProbeInstance probe;
  InstanceGate gate(probe.instance());

  gate.callFunctionOnModule("Probe", "say", folly::dynamic::array("open"));
  gate.close();
  gate.callFunctionOnModule("Probe", "say", folly::dynamic::array("closed"));

  CHECK(probe.heard() == std::vector<std::string>{"open"});
}

} // namespace rntizen
