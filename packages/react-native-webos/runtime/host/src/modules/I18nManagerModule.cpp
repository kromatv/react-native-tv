#include "I18nManagerModule.h"

#include "PlatformModuleRegistry.h"

#include <algorithm>

namespace rnwebos {

namespace {

std::string withUnderscores(std::string languageTag) {
  std::ranges::replace(languageTag, '-', '_');
  return languageTag;
}

} // namespace

I18nManagerModule::I18nManagerModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context)
    : NativeI18nManagerCxxSpec(std::move(jsInvoker)), localeIdentifier_(withUnderscores(context.device.locale)) {}

facebook::jsi::Object I18nManagerModule::getConstants(facebook::jsi::Runtime &runtime) {
  facebook::jsi::Object constants(runtime);
  constants.setProperty(runtime, "isRTL", false);
  constants.setProperty(runtime, "doLeftAndRightSwapInRTL", true);
  if (!localeIdentifier_.empty()) {
    constants.setProperty(runtime, "localeIdentifier", facebook::jsi::String::createFromUtf8(runtime, localeIdentifier_));
  }
  return constants;
}

void I18nManagerModule::allowRTL(facebook::jsi::Runtime &, bool) {}

void I18nManagerModule::forceRTL(facebook::jsi::Runtime &, bool) {}

void I18nManagerModule::swapLeftAndRightInRTL(facebook::jsi::Runtime &, bool) {}

RN_WEBOS_PLATFORM_MODULE("I18nManager", I18nManagerModule);

} // namespace rnwebos
