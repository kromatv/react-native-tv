#include "ImageLoaderModule.h"

#include "PlatformModuleRegistry.h"

#include "Promises.h"

namespace rntizen {

namespace {

constexpr const char *kGetSizeUnsupported = "Image.getSize is not supported on Tizen yet";

} // namespace

ImageLoaderModule::ImageLoaderModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &)
    : NativeImageLoaderAndroidCxxSpec(std::move(jsInvoker)) {}

void ImageLoaderModule::abortRequest(facebook::jsi::Runtime &, double) {}

facebook::jsi::Object ImageLoaderModule::getConstants(facebook::jsi::Runtime &runtime) {
  return facebook::jsi::Object(runtime);
}

facebook::jsi::Value ImageLoaderModule::getSize(facebook::jsi::Runtime &runtime, facebook::jsi::String) {
  return rejectedPromise(runtime, kGetSizeUnsupported);
}

facebook::jsi::Value ImageLoaderModule::getSizeWithHeaders(
    facebook::jsi::Runtime &runtime,
    facebook::jsi::String,
    facebook::jsi::Object) {
  return rejectedPromise(runtime, kGetSizeUnsupported);
}

facebook::jsi::Value ImageLoaderModule::prefetchImage(facebook::jsi::Runtime &runtime, facebook::jsi::String, double) {
  return resolvedPromise(runtime, facebook::jsi::Value(true));
}

facebook::jsi::Value ImageLoaderModule::queryCache(facebook::jsi::Runtime &runtime, facebook::jsi::Array) {
  return resolvedPromise(runtime, facebook::jsi::Value(runtime, facebook::jsi::Object(runtime)));
}

RN_TIZEN_PLATFORM_MODULE("ImageLoader", ImageLoaderModule);

} // namespace rntizen
