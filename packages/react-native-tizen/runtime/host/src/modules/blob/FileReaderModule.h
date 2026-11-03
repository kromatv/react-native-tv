#pragma once

#include "../ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>

#include <string>

namespace rntizen {

/// `FileReaderModule`, under React Native's `FileReader` and therefore
/// whatwg-fetch's `text()` and `json()`: reads a blob from the BlobStore as a
/// data URL, or as text in UTF-8, ISO-8859-1 or US-ASCII.
class FileReaderModule final : public facebook::react::NativeFileReaderModuleCxxSpec<FileReaderModule> {
 public:
  FileReaderModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  facebook::jsi::Value readAsDataURL(facebook::jsi::Runtime &runtime, facebook::jsi::Object data);
  facebook::jsi::Value readAsText(facebook::jsi::Runtime &runtime, facebook::jsi::Object data, std::string encoding);
};

} // namespace rntizen
