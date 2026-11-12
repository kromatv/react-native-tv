#pragma once

#include "../ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>
#include <react/bridging/Function.h>

#include <optional>
#include <string>

namespace rntizen {

/// `DialogManagerAndroid`, behind `Alert.alert`: the title, the message and up
/// to three buttons (neutral, negative, positive, left to right) in a native
/// alert the remote drives. A list of `items` is not supported.
class DialogManagerModule final : public facebook::react::NativeDialogManagerAndroidCxxSpec<DialogManagerModule> {
 public:
  DialogManagerModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  facebook::jsi::Object getConstants(facebook::jsi::Runtime &runtime);
  void showAlert(
      facebook::jsi::Runtime &runtime,
      facebook::jsi::Object config,
      facebook::react::AsyncCallback<std::string> onError,
      facebook::react::AsyncCallback<std::string, std::optional<int>> onAction);

 private:
  std::function<void(AlertRequest request)> showAlert_;
};

} // namespace rntizen
