#include "Context.h"

#include "AppDirectories.h"
#include "ExpoModuleRegistry.h"
#include "ModuleDefinition.h"
#include "views/ViewPrototypes.h"

#include "logging/Log.h"

#include <EventEmitter.h>
#include <NativeModule.h>

namespace rntizen::expo {

namespace {

using facebook::jsi::Object;
using facebook::jsi::Runtime;
using facebook::jsi::Value;

void emitOn(Runtime &runtime, const Object &emitter, const std::string &event, const JsValueFactory &payload) {
  Value argument = payload ? payload(runtime) : Value::undefined();
  ::expo::EventEmitter::emitEvent(runtime, emitter, event, &argument, 1);
}

} // namespace

Context::Context(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, ModuleContext host)
    : jsInvoker_(std::move(jsInvoker)), host_(std::move(host)), resourceDirectory_(appDirectories().resource) {}

const ModuleContext &Context::host() const {
  return host_;
}

const std::string &Context::resourceDirectory() const {
  return resourceDirectory_;
}

void Context::runOnJs(std::function<void(Runtime &runtime)> task) const {
  jsInvoker_->invokeAsync([task = std::move(task)](Runtime &runtime) {
    try {
      task(runtime);
    } catch (const std::exception &error) {
      log(LogLevel::Error, std::string("An Expo callback failed on the JavaScript thread: ") + error.what());
    }
  });
}

void Context::settlePromise(std::uint64_t id, PromiseOutcome outcome) const {
  runOnJs([id, outcome = std::move(outcome)](Runtime &runtime) { RuntimeState::get(runtime).settle(runtime, id, outcome); });
}

void Context::emit(const std::string &moduleName, const std::string &event, JsValueFactory payload) const {
  runOnJs([moduleName, event, payload = std::move(payload)](Runtime &runtime) {
    if (auto module = RuntimeState::get(runtime).loadedModule(runtime, moduleName)) {
      emitOn(runtime, *module, event, payload);
    }
  });
}

void Context::emit(const std::shared_ptr<SharedObject> &object, const std::string &event, JsValueFactory payload) const {
  const auto id = object->sharedObjectId();
  if (id == 0) return;
  runOnJs([id, event, payload = std::move(payload)](Runtime &runtime) {
    if (auto target = RuntimeState::get(runtime).liveObject(runtime, id)) emitOn(runtime, *target, event, payload);
  });
}

std::optional<Object> Context::createModuleObject(Runtime &runtime, const std::string &name) {
  std::shared_ptr<Module> module;
  {
    std::lock_guard lock(mutex_);
    if (auto found = modules_.find(name); found != modules_.end()) {
      module = found->second;
    } else {
      module = ExpoModuleRegistry::shared().create(name);
      if (!module) return std::nullopt;
      module->context_ = this;
      module->name_ = name;
      modules_.emplace(name, module);
    }
  }
  const auto definition = module->definition();
  if (definition.name() != name) {
    log(LogLevel::Warning, "The Expo module registered as " + name + " defines itself as " + definition.name());
  }
  auto object = ::expo::NativeModule::createInstance(runtime);
  definition.install(runtime, shared_from_this(), object);
  installViewPrototypes(runtime, shared_from_this(), object, name);
  return object;
}

JsValueFactory dynamicPayload(folly::dynamic payload) {
  return [payload = std::move(payload)](Runtime &runtime) { return facebook::jsi::valueFromDynamic(runtime, payload); };
}

void Module::sendEvent(const std::string &event, folly::dynamic payload) const {
  context_->emit(name_, event, dynamicPayload(std::move(payload)));
}

void Module::sendEvent(const std::shared_ptr<SharedObject> &object, const std::string &event, folly::dynamic payload)
    const {
  context_->emit(object, event, dynamicPayload(std::move(payload)));
}

} // namespace rntizen::expo
