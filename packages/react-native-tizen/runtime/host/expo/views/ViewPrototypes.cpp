#include "ViewPrototypes.h"

#include "ExpoView.h"
#include "ExpoViewRegistry.h"

#include "../Context.h"
#include "../RuntimeState.h"

#include "threading/MainThread.h"

#include <format>
#include <optional>

namespace rntizen::expo {

namespace {

using facebook::jsi::Function;
using facebook::jsi::Object;
using facebook::jsi::PropNameID;
using facebook::jsi::Runtime;
using facebook::jsi::Value;

std::optional<facebook::react::Tag> nativeTagOf(Runtime &runtime, const Value &component) {
  if (!component.isObject()) return std::nullopt;
  auto tag = component.getObject(runtime).getProperty(runtime, "nativeTag");
  if (!tag.isNumber()) return std::nullopt;
  return static_cast<facebook::react::Tag>(tag.getNumber());
}

PromiseOutcome callOnView(facebook::react::Tag tag, const std::string &name, const ExpoViewFunction &body) {
  auto *view = ExpoView::find(tag);
  if (view == nullptr) {
    return {.value = nullptr, .error = std::format("{}: no mounted view has tag {}", name, tag), .code = "ERR_VIEW"};
  }
  try {
    body(*view);
  } catch (const std::exception &error) {
    return {.value = nullptr, .error = std::format("{}: {}", name, error.what()), .code = ""};
  }
  return {};
}

Value callViewFunction(
    Runtime &runtime,
    const Value &thisValue,
    const std::shared_ptr<Context> &context,
    const std::string &name,
    const ExpoViewFunction &body) {
  auto &state = RuntimeState::get(runtime);
  auto created = state.createPromise(runtime);
  const auto tag = nativeTagOf(runtime, thisValue);
  if (!tag.has_value()) {
    state.settle(
        runtime, created.id, {.value = nullptr, .error = std::format("{}: the view is not mounted", name), .code = "ERR_VIEW"});
    return std::move(created.promise);
  }
  runOnMainThread([context, name, body, tag = *tag, id = created.id] { context->settlePromise(id, callOnView(tag, name, body)); });
  return std::move(created.promise);
}

Function viewFunction(
    Runtime &runtime, const std::shared_ptr<Context> &context, const std::string &name, ExpoViewFunction body) {
  return Function::createFromHostFunction(
      runtime,
      PropNameID::forUtf8(runtime, name),
      0,
      [context, name, body = std::move(body)](Runtime &rt, const Value &thisValue, const Value *, size_t) {
        return callViewFunction(rt, thisValue, context, name, body);
      });
}

} // namespace

void installViewPrototypes(
    Runtime &runtime, const std::shared_ptr<Context> &context, const Object &moduleObject, const std::string &moduleName) {
  const auto types = expoViewTypes(moduleName);
  if (types.empty()) return;
  Object prototypes(runtime);
  for (const auto &type : types) {
    Object prototype(runtime);
    for (const auto &[name, body] : type->functions) {
      prototype.setProperty(runtime, name.c_str(), viewFunction(runtime, context, name, body));
    }
    const auto key = type->viewName.empty() ? moduleName : moduleName + "_" + type->viewName;
    prototypes.setProperty(runtime, key.c_str(), prototype);
  }
  moduleObject.setProperty(runtime, "ViewPrototypes", prototypes);
}

} // namespace rntizen::expo
