#include "Context.h"
#include "ExpoRuntime.h"

#include "modules/PlatformModuleRegistry.h"

#include <ReactCommon/TurboModule.h>

namespace rntizen::expo {

namespace {

class ExpoModulesCoreModule final : public facebook::react::TurboModule {
 public:
  ExpoModulesCoreModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &host)
      : TurboModule("ExpoModulesCore", jsInvoker), context_(std::make_shared<Context>(jsInvoker, host)) {
    methodMap_["installModules"] = MethodMetadata{.argCount = 0, .invoker = &installModules};
  }

 private:
  static facebook::jsi::Value installModules(
      facebook::jsi::Runtime &runtime,
      TurboModule &module,
      const facebook::jsi::Value *,
      size_t) {
    installExpoGlobals(runtime, static_cast<ExpoModulesCoreModule &>(module).context_);
    return facebook::jsi::Value::undefined();
  }

  const std::shared_ptr<Context> context_;
};

} // namespace

RN_TIZEN_PLATFORM_MODULE("ExpoModulesCore", ExpoModulesCoreModule);

} // namespace rntizen::expo
