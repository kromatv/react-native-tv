#pragma once

#include <jsi/jsi.h>

#include <vector>

namespace rntizen::expo {

/// `globalThis.expo.modules`: each property is an Expo module from the
/// ExpoModuleRegistry, built on first read and kept for the runtime's life.
class ExpoModulesHostObject final : public facebook::jsi::HostObject {
 public:
  facebook::jsi::Value get(facebook::jsi::Runtime &runtime, const facebook::jsi::PropNameID &name) override;
  [[noreturn]] void set(
      facebook::jsi::Runtime &runtime,
      const facebook::jsi::PropNameID &name,
      const facebook::jsi::Value &value) override;
  std::vector<facebook::jsi::PropNameID> getPropertyNames(facebook::jsi::Runtime &runtime) override;
};

} // namespace rntizen::expo
