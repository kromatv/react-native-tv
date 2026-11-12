#pragma once

#include "ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>

namespace rntizen {

/// `ImageLoader`, which `<Image>` requires as soon as it loads. Images are
/// decoded by the Image view itself, so prefetching succeeds without work and
/// the cache is always empty. `Image.getSize` is not supported yet.
class ImageLoaderModule final : public facebook::react::NativeImageLoaderAndroidCxxSpec<ImageLoaderModule> {
 public:
  ImageLoaderModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  void abortRequest(facebook::jsi::Runtime &runtime, double requestId);
  facebook::jsi::Object getConstants(facebook::jsi::Runtime &runtime);
  facebook::jsi::Value getSize(facebook::jsi::Runtime &runtime, facebook::jsi::String uri);
  facebook::jsi::Value getSizeWithHeaders(
      facebook::jsi::Runtime &runtime,
      facebook::jsi::String uri,
      facebook::jsi::Object headers);
  facebook::jsi::Value prefetchImage(facebook::jsi::Runtime &runtime, facebook::jsi::String uri, double requestId);
  facebook::jsi::Value queryCache(facebook::jsi::Runtime &runtime, facebook::jsi::Array uris);
};

} // namespace rntizen
