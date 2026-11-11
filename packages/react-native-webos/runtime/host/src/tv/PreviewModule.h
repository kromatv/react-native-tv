#pragma once

#include "../modules/ModuleContext.h"

#include <ReactCommon/TurboModule.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace rntizen {

/// `RNTizenSmartHubPreview`: publishes Samsung's Smart Hub preview through the
/// shell, keeps the preview and its source in the data directory, and says
/// whether the app was launched from a preview tile, and why it cannot publish. `publish` settles its
/// promise once the main loop has handed the preview to the shell.
class PreviewModule final : public facebook::react::TurboModule, public std::enable_shared_from_this<PreviewModule> {
 public:
  PreviewModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

 private:
  using Runtime = facebook::jsi::Runtime;
  using Value = facebook::jsi::Value;

  struct Pending {
    facebook::jsi::Function resolve;
    facebook::jsi::Function reject;
  };

  static PreviewModule &self(TurboModule &turboModule);
  static Value isAvailable(Runtime &runtime, TurboModule &turboModule, const Value *arguments, size_t count);
  static Value unavailableReason(Runtime &runtime, TurboModule &turboModule, const Value *arguments, size_t count);
  static Value launchedFromTile(Runtime &runtime, TurboModule &turboModule, const Value *arguments, size_t count);
  static Value setSource(Runtime &runtime, TurboModule &turboModule, const Value *arguments, size_t count);
  static Value publish(Runtime &runtime, TurboModule &turboModule, const Value *arguments, size_t count);

  void settle(std::uint64_t id, int channel);
  std::optional<std::string> whyUnavailable() const;

  const std::shared_ptr<facebook::react::CallInvoker> invoker_;
  const std::shared_ptr<TvState> state_;
  const std::string platformVersion_;
  std::uint64_t nextId_{1};
  std::unordered_map<std::uint64_t, Pending> pending_;
};

} // namespace rntizen
