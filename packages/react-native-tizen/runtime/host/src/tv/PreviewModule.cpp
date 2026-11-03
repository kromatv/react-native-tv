#include "PreviewModule.h"

#include "../logging/Log.h"
#include "../modules/PlatformModuleRegistry.h"
#include "../threading/MainThread.h"
#include "PreviewFiles.h"
#include "TvState.h"

#include <array>

namespace rntizen {

namespace {

constexpr std::array<const char *, 3> kChannelNames{"none", "ContentDBAPI", "message port"};

} // namespace

using facebook::jsi::Function;
using facebook::jsi::Runtime;
using facebook::jsi::String;
using facebook::jsi::Value;
using facebook::react::TurboModule;

PreviewModule::PreviewModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context)
    : TurboModule("RNTizenSmartHubPreview", jsInvoker), invoker_(std::move(jsInvoker)), state_(context.tv) {
  methodMap_["isAvailable"] = MethodMetadata{.argCount = 0, .invoker = &isAvailable};
  methodMap_["launchedFromTile"] = MethodMetadata{.argCount = 0, .invoker = &launchedFromTile};
  methodMap_["publish"] = MethodMetadata{.argCount = 1, .invoker = &publish};
  methodMap_["setSource"] = MethodMetadata{.argCount = 1, .invoker = &setSource};
}

PreviewModule &PreviewModule::self(TurboModule &turboModule) {
  return static_cast<PreviewModule &>(turboModule);
}

Value PreviewModule::isAvailable(Runtime &, TurboModule &turboModule, const Value *, size_t) {
  const auto &state = self(turboModule).state_;
  return Value(state != nullptr && state->previewPublisher() != nullptr);
}

Value PreviewModule::launchedFromTile(Runtime &, TurboModule &turboModule, const Value *, size_t) {
  const auto &state = self(turboModule).state_;
  return Value(state != nullptr && state->launchedFromTile());
}

Value PreviewModule::setSource(Runtime &runtime, TurboModule &, const Value *arguments, size_t count) {
  if (count > 0 && arguments[0].isString()) {
    writePreviewSource(arguments[0].getString(runtime).utf8(runtime));
  } else {
    writePreviewSource(std::nullopt);
  }
  return Value::undefined();
}

Value PreviewModule::publish(Runtime &runtime, TurboModule &turboModule, const Value *arguments, size_t count) {
  if (count == 0 || !arguments[0].isString()) throw facebook::jsi::JSError(runtime, "publish takes a JSON string");
  auto &preview = self(turboModule);
  auto json = arguments[0].getString(runtime).utf8(runtime);
  const auto id = preview.nextId_++;
  auto executor = Function::createFromHostFunction(
      runtime,
      facebook::jsi::PropNameID::forAscii(runtime, "publish"),
      2,
      [&preview, id](Runtime &runtime, const Value &, const Value *settle, size_t) {
        preview.pending_.try_emplace(
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
    if (auto previewModule = weak.lock()) previewModule->settle(id, channel);
  });
  return promise;
}

void PreviewModule::settle(std::uint64_t id, int channel) {
  invoker_->invokeAsync([weak = weak_from_this(), id, channel](Runtime &runtime) {
    auto previewModule = weak.lock();
    if (!previewModule) return;
    auto found = previewModule->pending_.find(id);
    if (found == previewModule->pending_.end()) return;
    auto pending = std::move(found->second);
    previewModule->pending_.erase(found);
    if (channel > 0) {
      const auto *name = kChannelNames[static_cast<std::size_t>(channel) % kChannelNames.size()];
      log(LogLevel::Info, std::string("Smart Hub preview: published through ") + name);
      pending.resolve.call(runtime, String::createFromAscii(runtime, name));
      return;
    }
    const auto message = channel < 0 ? "Smart Hub preview needs a Samsung TV: this device has no Smart Hub"
                                     : "Smart Hub preview: the TV took the preview through no channel";
    auto error = runtime.global().getPropertyAsFunction(runtime, "Error").callAsConstructor(
        runtime, String::createFromAscii(runtime, message));
    pending.reject.call(runtime, error);
  });
}

RN_TIZEN_PLATFORM_MODULE("RNTizenSmartHubPreview", PreviewModule);

} // namespace rntizen
