#include "../logging/Log.h"
#include "../modules/PlatformModuleRegistry.h"
#include "../threading/MainThread.h"
#include "PreviewFiles.h"
#include "TvState.h"

#include <ReactCommon/TurboModule.h>

#include <cstdint>
#include <unordered_map>

namespace rntizen {

namespace {

using facebook::jsi::Function;
using facebook::jsi::Runtime;
using facebook::jsi::String;
using facebook::jsi::Value;
using facebook::react::TurboModule;

constexpr const char *kChannelNames[] = {"none", "ContentDBAPI", "message port"};

class PreviewModule final : public TurboModule, public std::enable_shared_from_this<PreviewModule> {
 public:
  PreviewModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context)
      : TurboModule("RNTizenSmartHubPreview", jsInvoker), invoker_(std::move(jsInvoker)), state_(context.tv) {
    methodMap_["isAvailable"] = MethodMetadata{.argCount = 0, .invoker = &isAvailable};
    methodMap_["launchedFromTile"] = MethodMetadata{.argCount = 0, .invoker = &launchedFromTile};
    methodMap_["publish"] = MethodMetadata{.argCount = 1, .invoker = &publish};
    methodMap_["setSource"] = MethodMetadata{.argCount = 1, .invoker = &setSource};
  }

 private:
  struct Pending {
    Function resolve;
    Function reject;
  };

  static PreviewModule &self(TurboModule &module) {
    return static_cast<PreviewModule &>(module);
  }

  static Value isAvailable(Runtime &, TurboModule &module, const Value *, size_t) {
    const auto &state = self(module).state_;
    return Value(state != nullptr && state->previewPublisher() != nullptr);
  }

  static Value launchedFromTile(Runtime &, TurboModule &module, const Value *, size_t) {
    const auto &state = self(module).state_;
    return Value(state != nullptr && state->launchedFromTile());
  }

  static Value setSource(Runtime &runtime, TurboModule &, const Value *arguments, size_t count) {
    if (count > 0 && arguments[0].isString()) {
      writePreviewSource(arguments[0].getString(runtime).utf8(runtime));
    } else {
      writePreviewSource(std::nullopt);
    }
    return Value::undefined();
  }

  static Value publish(Runtime &runtime, TurboModule &module, const Value *arguments, size_t count) {
    if (count == 0 || !arguments[0].isString()) throw facebook::jsi::JSError(runtime, "publish takes a JSON string");
    auto &preview = self(module);
    auto json = arguments[0].getString(runtime).utf8(runtime);
    const auto id = preview.nextId_++;
    auto executor = Function::createFromHostFunction(
        runtime,
        facebook::jsi::PropNameID::forAscii(runtime, "publish"),
        2,
        [&preview, id](Runtime &runtime, const Value &, const Value *settle, size_t) {
          preview.pending_.emplace(
              id,
              Pending{
                  .resolve = settle[0].getObject(runtime).getFunction(runtime),
                  .reject = settle[1].getObject(runtime).getFunction(runtime)});
          return Value::undefined();
        });
    auto promise = runtime.global().getPropertyAsFunction(runtime, "Promise").callAsConstructor(runtime, executor);
    writePreviewFile(json);
    auto publisher = preview.state_ != nullptr ? preview.state_->previewPublisher() : nullptr;
    runOnMainThread([weak = preview.weak_from_this(), publisher, json = std::move(json), id] {
      const int channel = publisher ? publisher(json) : -1;
      if (auto module = weak.lock()) module->settle(id, channel);
    });
    return promise;
  }

  void settle(std::uint64_t id, int channel) {
    invoker_->invokeAsync([weak = weak_from_this(), id, channel](Runtime &runtime) {
      auto module = weak.lock();
      if (!module) return;
      auto found = module->pending_.find(id);
      if (found == module->pending_.end()) return;
      auto pending = std::move(found->second);
      module->pending_.erase(found);
      if (channel > 0) {
        log(LogLevel::Info, std::string("Smart Hub preview: published through ") + kChannelNames[channel % 3]);
        pending.resolve.call(runtime, String::createFromAscii(runtime, kChannelNames[channel % 3]));
        return;
      }
      const auto message = channel < 0 ? "Smart Hub preview needs a Samsung TV: this device has no Smart Hub"
                                       : "Smart Hub preview: the TV took the preview through no channel";
      auto error = runtime.global().getPropertyAsFunction(runtime, "Error").callAsConstructor(
          runtime, String::createFromAscii(runtime, message));
      pending.reject.call(runtime, error);
    });
  }

  const std::shared_ptr<facebook::react::CallInvoker> invoker_;
  const std::shared_ptr<TvState> state_;
  std::uint64_t nextId_{1};
  std::unordered_map<std::uint64_t, Pending> pending_;
};

} // namespace

RN_TIZEN_PLATFORM_MODULE("RNTizenSmartHubPreview", PreviewModule);

} // namespace rntizen
