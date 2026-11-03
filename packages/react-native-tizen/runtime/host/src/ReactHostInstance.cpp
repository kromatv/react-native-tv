#include "ReactHost.h"

#include "bundle/BundleLoader.h"
#include "fabric/ComponentDescriptorRegistry.h"
#include "logging/Log.h"
#include "threading/MainThread.h"
#include "timers/ThreadTimerRegistry.h"

#include <ReactCommon/TurboModuleBinding.h>
#include <react/renderer/componentregistry/native/NativeComponentRegistryBinding.h>
#include <react/renderer/runtimescheduler/RuntimeScheduler.h>
#include <react/runtime/JSRuntimeBindings.h>
#include <react/runtime/hermes/HermesInstance.h>

#if RN_TIZEN_EXPO
#include "../expo/ExpoRuntime.h"
#endif

#include <format>
#include <sstream>

namespace rntizen {

namespace {

LogLevel javaScriptLogLevel(unsigned int level) {
  using enum LogLevel;
  switch (level) {
    case 0:
      return Debug;
    case 1:
      return Info;
    case 2:
      return Warning;
    default:
      return Error;
  }
}

std::string fileName(const std::optional<std::string> &path) {
  if (!path) return "?";
  const auto slash = path->find_last_of('/');
  return slash == std::string::npos ? *path : path->substr(slash + 1);
}

ErrorReport reportFor(const facebook::react::JsErrorHandler::ProcessedError &error) {
  ErrorReport report{
      .title = error.name.value_or("Error"),
      .message = error.originalMessage.value_or(error.message),
      .stack = {},
  };
  for (const auto &frame : error.stack) {
    report.stack.push_back(std::format(
        "{} ({}:{}:{})", frame.methodName, fileName(frame.file), frame.lineNumber.value_or(0), frame.column.value_or(0)));
  }
  return report;
}

} // namespace

void ReactHost::createInstance() {
  using namespace facebook::react;
  jsThread_ = std::make_shared<TaskQueueThread>("rn-js");

  auto timerRegistry = std::make_unique<ThreadTimerRegistry>();
  auto *timerRegistryHandle = timerRegistry.get();
  timerManager_ = std::make_shared<TimerManager>(std::move(timerRegistry));
  timerRegistryHandle->setTimerManager(timerManager_);

  auto onJsError = [weakHost = weakHost_](facebook::jsi::Runtime &, const JsErrorHandler::ProcessedError &error) {
    std::ostringstream description;
    description << error;
    log(error.isFatal ? LogLevel::Error : LogLevel::Warning, description.str());
    if (error.isFatal) reportFatalError(weakHost, reportFor(error));
  };
  instance_ = std::make_unique<ReactInstance>(
      HermesInstance::createJSRuntime(nullptr, jsThread_, false),
      jsThread_,
      timerManager_,
      std::move(onJsError),
      inspectorTarget());
  timerManager_->setRuntimeExecutor(instance_->getBufferedRuntimeExecutor());
  instanceGate_ = std::make_shared<InstanceGate>(*instance_);

  modules_ = std::make_shared<TurboModuleProvider>(
      ModuleContext{
          .device = device_,
          .scriptURL = config_.bundleUrl.value_or("file://" + config_.resourceDir + "/" + config_.bundleFile),
          .emitDeviceEvent =
              [gate = instanceGate_](const std::string &eventName, folly::dynamic payload) {
                gate->callFunctionOnModule(
                    "RCTDeviceEventEmitter", "emit", folly::dynamic::array(eventName, std::move(payload)));
              },
          .exitApp =
              [weakHost = weakHost_] {
                runOnMainThread([weakHost] {
                  if (auto host = weakHost.lock()) (*host)->exit();
                });
              },
          .isActive = isActive_,
          .reportFatalError = [weakHost = weakHost_](const ErrorReport &report) { reportFatalError(weakHost, report); },
          .launchUrl = launchUrl_,
          .showAlert =
              [weakHost = weakHost_](AlertRequest request) {
                runOnMainThread([weakHost, request = std::move(request)]() mutable {
                  if (auto host = weakHost.lock()) {
                    (*host)->alertDialog_->show(
                        std::move(request), (*host)->device_.screenWidth, (*host)->device_.screenHeight);
                  }
                });
              },
          .tv = tv_.state,
      },
      instance_->createJSCallInvoker());

  auto contextContainer = std::make_shared<ContextContainer>();
  contextContainer->insert(RuntimeSchedulerKey, std::weak_ptr<RuntimeScheduler>(instance_->getRuntimeScheduler()));
  contextContainer->insert(TextMeasurer::contextKey, std::shared_ptr<const TextMeasurer>(textMeasurer_));
  presenter_ = std::make_unique<SurfacePresenter>(
      evas_object_evas_get(window_),
      SurfacePresenter::Dependencies{
          .contextContainer = contextContainer,
          .runtimeScheduler = instance_->getRuntimeScheduler(),
          .runtimeExecutor = instance_->getBufferedRuntimeExecutor(),
          .bridgelessBindingsExecutor = instance_->getUnbufferedRuntimeExecutor(),
      });

  instance_->initializeRuntime({.isProfiling = false}, [modules = modules_](facebook::jsi::Runtime &runtime) {
    TurboModuleBinding::install(
        runtime, [modules](facebook::jsi::Runtime &, const std::string &name) { return modules->get(name); });
    bindNativeLogger(runtime, [](const std::string &message, unsigned int level) {
      log(javaScriptLogLevel(level), message);
    });
    bindHasComponentProvider(runtime, [](const std::string &name) { return hasComponent(name); });
#if RN_TIZEN_EXPO
    expo::installExpoRuntime(runtime);
#endif
  });
}

void ReactHost::loadBundle() {
  if (config_.bundleUrl) {
    devSupportInstanceStarted();
    jsThread_->runOnQueue([this, url = *config_.bundleUrl] {
      try {
        auto bundle = downloadBundle(url);
        instance_->loadScript(std::move(bundle.script), bundle.sourceURL);
      } catch (const std::exception &error) {
        log(LogLevel::Error, error.what());
        reportFatalError(
            weakHost_, {.title = "Cannot load the JavaScript bundle", .message = error.what(), .stack = {}});
      }
    });
    return;
  }
  auto bundle = loadBundleFile(config_.resourceDir + "/" + config_.bundleFile);
  instance_->loadScript(std::move(bundle.script), bundle.sourceURL);
}

void ReactHost::reportFatalError(const std::weak_ptr<ReactHost *> &weakHost, ErrorReport report) {
  runOnMainThread([weakHost, report = std::move(report)] {
    if (auto host = weakHost.lock()) (*host)->showFatalError(report);
  });
}

void ReactHost::showFatalError(const ErrorReport &report) {
  if (!config_.development) {
    exit();
    return;
  }
  errorScreen_->show(report, device_.screenWidth, device_.screenHeight);
}

} // namespace rntizen
