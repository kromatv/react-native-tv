#include "LinkingManagerModule.h"

#include "../platform/LunaBus.h"
#include "PlatformModuleRegistry.h"
#include "Promises.h"

#include <react/bridging/Error.h>

namespace rnwebos {

namespace {

constexpr const char *kLaunch = "luna://com.webos.applicationManager/launch";
constexpr const char *kBrowser = "com.webos.app.browser";

bool isWebUrl(const std::string &url) {
  return url.starts_with("http://") || url.starts_with("https://");
}

std::string refusal(const folly::dynamic &reply) {
  const auto *text = reply.isObject() ? reply.get_ptr("errorText") : nullptr;
  return text != nullptr && text->isString() ? text->getString() : "the app manager refused";
}

bool succeeded(const folly::dynamic &reply) {
  const auto *returnValue = reply.isObject() ? reply.get_ptr("returnValue") : nullptr;
  return returnValue != nullptr && returnValue->isBool() && returnValue->getBool();
}

} // namespace

LinkingManagerModule::LinkingManagerModule(
    std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
    const ModuleContext &context)
    : NativeLinkingManagerCxxSpec(std::move(jsInvoker)), launch_(context.launch), luna_(context.luna) {}

bool LinkingManagerModule::opens(const std::string &url) const {
  return isWebUrl(url) && luna_ != nullptr && luna_->isAvailable();
}

facebook::jsi::Value LinkingManagerModule::getInitialURL(facebook::jsi::Runtime &runtime) {
  auto url = launch_ != nullptr ? launch_->initialUrl() : std::nullopt;
  if (!url) return resolvedPromise(runtime, facebook::jsi::Value::null());
  return resolvedPromise(runtime, facebook::jsi::String::createFromUtf8(runtime, *url));
}

facebook::jsi::Value LinkingManagerModule::canOpenURL(facebook::jsi::Runtime &runtime, facebook::jsi::String url) {
  return resolvedPromise(runtime, facebook::jsi::Value(opens(url.utf8(runtime))));
}

facebook::react::AsyncPromise<> LinkingManagerModule::openURL(facebook::jsi::Runtime &runtime, facebook::jsi::String url) {
  facebook::react::AsyncPromise<> promise(runtime, jsInvoker_);
  const auto target = url.utf8(runtime);
  if (!opens(target)) {
    promise.reject(facebook::react::Error(
        "Cannot open " + target +
        (isWebUrl(target) ? ": the TV's app manager cannot be reached" : ": webOS opens http and https URLs only, in its browser")));
    return promise;
  }
  const folly::dynamic request = folly::dynamic::object("id", kBrowser)("params", folly::dynamic::object("target", target));
  const bool asked = luna_->call(kLaunch, request, [promise, target](const folly::dynamic &reply) mutable {
    if (succeeded(reply)) return promise.resolve();
    promise.reject(facebook::react::Error("Cannot open " + target + ": " + refusal(reply)));
  });
  if (!asked) promise.reject(facebook::react::Error("Cannot open " + target + ": the TV's app manager cannot be reached"));
  return promise;
}

facebook::jsi::Value LinkingManagerModule::openSettings(facebook::jsi::Runtime &runtime) {
  return rejectedPromise(runtime, "Linking.openSettings is not supported on webOS");
}

void LinkingManagerModule::addListener(facebook::jsi::Runtime &, facebook::jsi::String) {}

void LinkingManagerModule::removeListeners(facebook::jsi::Runtime &, double) {}

RN_WEBOS_PLATFORM_MODULE("LinkingManager", LinkingManagerModule);

} // namespace rnwebos
