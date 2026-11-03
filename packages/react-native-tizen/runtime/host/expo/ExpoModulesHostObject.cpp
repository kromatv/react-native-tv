#include "ExpoModulesHostObject.h"

#include "ExpoModuleRegistry.h"
#include "RuntimeState.h"

namespace rntizen::expo {

facebook::jsi::Value ExpoModulesHostObject::get(facebook::jsi::Runtime &runtime, const facebook::jsi::PropNameID &name) {
  return RuntimeState::get(runtime).module(runtime, name.utf8(runtime));
}

void ExpoModulesHostObject::set(
    facebook::jsi::Runtime &runtime,
    const facebook::jsi::PropNameID &name,
    const facebook::jsi::Value &) {
  throw facebook::jsi::JSError(runtime, "Cannot override the Expo module '" + name.utf8(runtime) + "'");
}

std::vector<facebook::jsi::PropNameID> ExpoModulesHostObject::getPropertyNames(facebook::jsi::Runtime &runtime) {
  std::vector<facebook::jsi::PropNameID> names;
  for (const auto &name : ExpoModuleRegistry::shared().names()) {
    names.push_back(facebook::jsi::PropNameID::forUtf8(runtime, name));
  }
  return names;
}

} // namespace rntizen::expo
