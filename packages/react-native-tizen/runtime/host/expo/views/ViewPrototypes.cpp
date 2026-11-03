#include "ViewPrototypes.h"

#include "ExpoView.h"
#include "ExpoViewRegistry.h"

#include "../Context.h"
#include "../RuntimeState.h"

#include "threading/MainThread.h"

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

Function viewFunction(
    Runtime &runtime, const std::shared_ptr<Context> &context, const std::string &name, ExpoViewFunction body) {
  return Function::createFromHostFunction(
      runtime,
      PropNameID::forUtf8(runtime, name),
      0,
      [context, name, body = std::move(body)](Runtime &rt, const Value &thisValue, const Value *, size_t) -> Value {
        auto &state = RuntimeState::get(rt);
        auto created = state.createPromise(rt);
        const auto tag = nativeTagOf(rt, thisValue);
        if (!tag) {
          state.settle(rt, created.id, {.value = nullptr, .error = name + ": the view is not mounted", .code = "ERR_VIEW"});
          return std::move(created.promise);
        }
        runOnMainThread([context, name, body, tag = *tag, id = created.id] {
          PromiseOutcome outcome;
          if (auto *view = ExpoView::find(tag)) {
            try {
              body(*view);
            } catch (const std::exception &error) {
              outcome.error = name + ": " + error.what();
            }
          } else {
            outcome.error = name + ": no mounted view has tag " + std::to_string(tag);
            outcome.code = "ERR_VIEW";
          }
          context->settlePromise(id, std::move(outcome));
        });
        return std::move(created.promise);
      });
}

} // namespace

void installViewPrototypes(
    Runtime &runtime, const std::shared_ptr<Context> &context, Object &module, const std::string &moduleName) {
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
  module.setProperty(runtime, "ViewPrototypes", prototypes);
}

} // namespace rntizen::expo
