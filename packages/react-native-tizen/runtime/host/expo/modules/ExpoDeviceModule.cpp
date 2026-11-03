#include "../ExpoModuleRegistry.h"
#include "../ModuleDefinition.h"
#include "DeviceName.h"

#include <sys/utsname.h>
#include <unistd.h>

#include <chrono>
#include <ctime>

namespace rntizen::expo {

namespace {

constexpr int kDeviceTypeTv = 4;

std::optional<double> totalMemory() {
  const long pages = sysconf(_SC_PHYS_PAGES);
  const long pageSize = sysconf(_SC_PAGESIZE);
  if (pages <= 0 || pageSize <= 0) return std::nullopt;
  return static_cast<double>(pages) * static_cast<double>(pageSize);
}

std::vector<std::string> cpuArchitectures() {
  utsname system{};
  if (uname(&system) != 0) return {};
  return {system.machine};
}

double uptimeMilliseconds() {
  timespec time{};
  clock_gettime(CLOCK_BOOTTIME, &time);
  return static_cast<double>(time.tv_sec) * 1000.0 + static_cast<double>(time.tv_nsec) / 1e6;
}

class ExpoDeviceModule final : public Module {
 public:
  ModuleDefinition definition() override {
    const auto &device = context().host().device;
    const std::optional<std::string> none;
    return ModuleDefinition("ExpoDevice")
        .constant("isDevice", [] { return true; })
        .constant("brand", [] { return std::string("Samsung"); })
        .constant("manufacturer", [] { return std::string("Samsung"); })
        .constant("modelId", [none] { return none; })
        .constant("modelName", [&device] { return device.modelName; })
        .constant("designName", [none] { return none; })
        .constant("productName", [none] { return none; })
        .constant("deviceType", [] { return kDeviceTypeTv; })
        .constant("deviceYearClass", [] { return std::optional<int>(); })
        .constant("totalMemory", [] { return totalMemory(); })
        .constant("supportedCpuArchitectures", [] { return cpuArchitectures(); })
        .constant("osName", [] { return std::string("Tizen"); })
        .constant("osVersion", [&device] { return device.platformVersion; })
        .constant("osBuildId", [&device] { return device.buildString; })
        .constant("osInternalBuildId", [&device] { return device.buildString; })
        .constant("osBuildFingerprint", [none] { return none; })
        .constant("platformApiLevel", [] { return std::optional<int>(); })
        .constant("deviceName", [&device] { return tvDeviceName(device.modelName); })
        .asyncFunction("getDeviceTypeAsync", [] { return kDeviceTypeTv; })
        .asyncFunction("getUptimeAsync", [] { return uptimeMilliseconds(); })
        .asyncFunction("isRootedExperimentalAsync", [] { return false; })
        .asyncFunction("isSideLoadingEnabledAsync", [] { return false; })
        .asyncFunction("getPlatformFeaturesAsync", [] { return std::vector<std::string>(); })
        .asyncFunction("hasPlatformFeatureAsync", [](std::string) { return false; });
  }
};

} // namespace

RN_TIZEN_EXPO_MODULE(ExpoDevice, ExpoDeviceModule);

} // namespace rntizen::expo
