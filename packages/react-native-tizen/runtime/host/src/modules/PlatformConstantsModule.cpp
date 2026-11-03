#include "PlatformConstantsModule.h"

#include "PlatformModuleRegistry.h"

#include <cxxreact/ReactNativeVersion.h>
#include <jsi/JSIDynamic.h>

#include <cstdlib>

namespace rntizen {

namespace {

int majorVersion(const std::string &version) {
  return std::atoi(version.c_str());
}

} // namespace

PlatformConstantsModule::PlatformConstantsModule(
    std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
    const ModuleContext &context)
    : NativePlatformConstantsAndroidCxxSpec(std::move(jsInvoker)), device_(context.device) {}

facebook::jsi::Object PlatformConstantsModule::getConstants(facebook::jsi::Runtime &runtime) {
  using facebook::react::ReactNativeVersion;
  folly::dynamic version = folly::dynamic::object("major", ReactNativeVersion.Major)("minor", ReactNativeVersion.Minor)(
      "patch", ReactNativeVersion.Patch)(
      "prerelease",
      ReactNativeVersion.Prerelease.empty() ? folly::dynamic(nullptr)
                                            : folly::dynamic(std::string(ReactNativeVersion.Prerelease)));
  folly::dynamic constants = folly::dynamic::object("isTesting", false)("reactNativeVersion", std::move(version))(
      "Version", majorVersion(device_.platformVersion))("Release", device_.platformVersion)("Serial", "unknown")(
      "Fingerprint", device_.buildString)("Model", device_.modelName)("uiMode", "tv")("Brand", "Samsung")(
      "Manufacturer", "Samsung");
  return facebook::jsi::valueFromDynamic(runtime, constants).asObject(runtime);
}

facebook::jsi::String PlatformConstantsModule::getAndroidID(facebook::jsi::Runtime &runtime) {
  return facebook::jsi::String::createFromAscii(runtime, "");
}

RN_TIZEN_PLATFORM_MODULE("PlatformConstants", PlatformConstantsModule);

} // namespace rntizen
