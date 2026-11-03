#pragma once

#include "ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>

namespace rntizen {

/// `SourceCode`: the URL the bundle was loaded from, which resolves asset URLs
/// against the dev server during development.
class SourceCodeModule final : public facebook::react::NativeSourceCodeCxxSpec<SourceCodeModule> {
 public:
  SourceCodeModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  facebook::jsi::Object getConstants(facebook::jsi::Runtime &runtime);

 private:
  std::string scriptURL_;
};

} // namespace rntizen
