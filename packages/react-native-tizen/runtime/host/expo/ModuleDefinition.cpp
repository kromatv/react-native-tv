#include "ModuleDefinition.h"

namespace rntizen::expo {

namespace detail {

using facebook::jsi::Function;
using facebook::jsi::Object;
using facebook::jsi::PropNameID;
using facebook::jsi::Runtime;
using facebook::jsi::Value;

void defineGetter(Runtime &runtime, Object &target, const std::string &name, facebook::jsi::HostFunctionType getter) {
  defineAccessor(runtime, target, name, std::move(getter), nullptr);
}

void defineAccessor(
    Runtime &runtime,
    Object &target,
    const std::string &name,
    facebook::jsi::HostFunctionType getter,
    facebook::jsi::HostFunctionType setter) {
  auto get = Function::createFromHostFunction(runtime, PropNameID::forUtf8(runtime, name), 0, std::move(getter));
  Object descriptor(runtime);
  descriptor.setProperty(runtime, "get", get);
  if (setter) {
    descriptor.setProperty(
        runtime, "set", Function::createFromHostFunction(runtime, PropNameID::forUtf8(runtime, name), 1, std::move(setter)));
  }
  descriptor.setProperty(runtime, "enumerable", true);
  descriptor.setProperty(runtime, "configurable", true);
  ::expo::common::defineProperty(runtime, &target, name.c_str(), std::move(descriptor));
}

void defineFunction(Runtime &runtime, Object &target, const std::string &name, facebook::jsi::HostFunctionType body) {
  target.setProperty(
      runtime, name.c_str(), Function::createFromHostFunction(runtime, PropNameID::forUtf8(runtime, name), 0, std::move(body)));
}

} // namespace detail

ModuleDefinition &ModuleDefinition::onStartObserving(std::function<void(const std::string &event)> body) {
  return function("startObserving", [body = std::move(body)](std::string event) { body(event); });
}

ModuleDefinition &ModuleDefinition::onStopObserving(std::function<void(const std::string &event)> body) {
  return function("stopObserving", [body = std::move(body)](std::string event) { body(event); });
}

void ModuleDefinition::install(
    facebook::jsi::Runtime &runtime,
    const std::shared_ptr<Context> &context,
    facebook::jsi::Object &module) const {
  for (const auto &member : members_) member(runtime, context, module);
}

} // namespace rntizen::expo
