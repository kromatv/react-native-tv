#include "FileReaderModule.h"

#include "../PlatformModuleRegistry.h"
#include "../Promises.h"
#include "BlobReference.h"

#include "../../network/Base64.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <optional>

namespace rntizen {

namespace {

using facebook::jsi::Runtime;

constexpr std::string_view kReplacementCharacter = "\uFFFD";

std::string lowercase(std::string text) {
  std::ranges::transform(text, text.begin(), [](unsigned char c) { return std::tolower(c); });
  return text;
}

std::string latin1ToUtf8(std::string_view bytes, bool asciiOnly) {
  std::string text;
  text.reserve(bytes.size());
  for (const char byte : bytes) {
    const auto code = static_cast<std::byte>(byte);
    if (std::to_integer<unsigned>(code) < 0x80) {
      text.push_back(byte);
    } else if (asciiOnly) {
      text.append(kReplacementCharacter);
    } else {
      text.push_back(std::to_integer<char>(std::byte{0xC0} | (code >> 6)));
      text.push_back(std::to_integer<char>(std::byte{0x80} | (code & std::byte{0x3F})));
    }
  }
  return text;
}

std::optional<std::string> decoded(std::string bytes, const std::string &encoding) {
  const auto name = lowercase(encoding);
  if (name.empty() || name == "utf-8" || name == "utf8") return bytes;
  if (name == "iso-8859-1" || name == "latin1" || name == "l1") return latin1ToUtf8(bytes, false);
  if (name == "us-ascii" || name == "ascii") return latin1ToUtf8(bytes, true);
  return std::nullopt;
}

} // namespace

FileReaderModule::FileReaderModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &)
    : NativeFileReaderModuleCxxSpec(std::move(jsInvoker)) {}

facebook::jsi::Value FileReaderModule::readAsDataURL(Runtime &runtime, facebook::jsi::Object data) {
  const auto reference = readBlobReference(runtime, data);
  auto bytes = resolveBlob(reference);
  if (!bytes) return rejectedPromise(runtime, "The specified blob is invalid");
  const auto type = reference.type.empty() ? std::string("application/octet-stream") : reference.type;
  const auto url = "data:" + type + ";base64," + encodeBase64(*bytes);
  return resolvedPromise(runtime, facebook::jsi::String::createFromUtf8(runtime, url));
}

facebook::jsi::Value FileReaderModule::readAsText(
    Runtime &runtime,
    facebook::jsi::Object data,
    const std::string &encoding) {
  auto bytes = resolveBlob(readBlobReference(runtime, data));
  if (!bytes) return rejectedPromise(runtime, "The specified blob is invalid");
  auto text = decoded(std::move(*bytes), encoding);
  if (!text) return rejectedPromise(runtime, "Unsupported encoding " + encoding);
  return resolvedPromise(runtime, facebook::jsi::String::createFromUtf8(runtime, *text));
}

RN_TIZEN_PLATFORM_MODULE("FileReaderModule", FileReaderModule);

} // namespace rntizen
