#include "SourceCodeModule.h"

#include "PlatformModuleRegistry.h"

namespace rnwebos {

SourceCodeModule::SourceCodeModule(
    std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
    const ModuleContext &context)
    : NativeSourceCodeCxxSpec(std::move(jsInvoker)), scriptURL_(context.scriptURL) {}

facebook::jsi::Object SourceCodeModule::getConstants(facebook::jsi::Runtime &runtime) {
  facebook::jsi::Object constants(runtime);
  constants.setProperty(runtime, "scriptURL", facebook::jsi::String::createFromUtf8(runtime, scriptURL_));
  return constants;
}

RN_WEBOS_PLATFORM_MODULE("SourceCode", SourceCodeModule);

} // namespace rnwebos
