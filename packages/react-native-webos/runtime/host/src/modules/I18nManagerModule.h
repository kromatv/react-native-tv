#pragma once

#include "ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>

namespace rnwebos {

/// `I18nManager`: `localeIdentifier` is the language of the TV's menus, as iOS
/// writes one (`en_GB`). The host lays out left to right only, so `isRTL` is
/// false and the setters change nothing.
class I18nManagerModule final : public facebook::react::NativeI18nManagerCxxSpec<I18nManagerModule> {
 public:
  I18nManagerModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  facebook::jsi::Object getConstants(facebook::jsi::Runtime &runtime);
  void allowRTL(facebook::jsi::Runtime &runtime, bool allowRTL);
  void forceRTL(facebook::jsi::Runtime &runtime, bool forceRTL);
  void swapLeftAndRightInRTL(facebook::jsi::Runtime &runtime, bool flipStyles);

 private:
  const std::string localeIdentifier_;
};

} // namespace rnwebos
