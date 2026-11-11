#include "RuntimeState.h"

#include "Context.h"
#include "SharedObjectRegistry.h"

#include <JSIUtils.h>
#include <SharedObject.h>

namespace rntizen::expo {

namespace {

using facebook::jsi::Function;
using facebook::jsi::JSError;
using facebook::jsi::Object;
using facebook::jsi::PropNameID;
using facebook::jsi::Runtime;
using facebook::jsi::String;
using facebook::jsi::Value;

constexpr facebook::jsi::UUID kStateKey{0x6d1f3a52, 0x8c2e, 0x4b0a, 0x9f31, 0x2b7c4e5d6a18};

void releaseSharedObject(const ::expo::SharedObject::ObjectId id) {
  SharedObjectRegistry::shared().release(id);
}

} // namespace

Value makeError(Runtime &runtime, const std::string &message, const std::string &code) {
  auto error = runtime.global().getPropertyAsFunction(runtime, "Error").callAsConstructor(
      runtime, String::createFromUtf8(runtime, message));
  if (!code.empty()) error.getObject(runtime).setProperty(runtime, "code", String::createFromUtf8(runtime, code));
  return error;
}

RuntimeState::RuntimeState(std::shared_ptr<Context> context) : context_(std::move(context)) {}

void RuntimeState::install(Runtime &runtime, std::shared_ptr<Context> context) {
  runtime.setRuntimeData(kStateKey, std::make_shared<RuntimeState>(std::move(context)));
}

RuntimeState &RuntimeState::get(Runtime &runtime) {
  auto state = runtime.getRuntimeData(kStateKey);
  if (!state) throw JSError(runtime, "Expo modules are not installed in this runtime");
  return *std::static_pointer_cast<RuntimeState>(state);
}

const std::shared_ptr<Context> &RuntimeState::context() const {
  return context_;
}

Value RuntimeState::moduleValue(Runtime &runtime, const std::string &name) {
  if (auto loaded = loadedModule(runtime, name)) return Value(runtime, *loaded);
  auto object = context_->createModuleObject(runtime, name);
  if (!object) return Value::undefined();
  auto value = Value(runtime, *object);
  modules_.try_emplace(name, std::move(*object));
  return value;
}

std::optional<Object> RuntimeState::loadedModule(Runtime &runtime, const std::string &name) {
  auto found = modules_.find(name);
  if (found == modules_.end()) return std::nullopt;
  return Value(runtime, found->second).getObject(runtime);
}

void RuntimeState::registerClass(Runtime &runtime, std::type_index type, const Function &klass) {
  classes_.insert_or_assign(type, Value(runtime, klass).getObject(runtime).getFunction(runtime));
}

void RuntimeState::pair(Runtime &runtime, const Object &object, std::shared_ptr<SharedObject> native) {
  forgetReleased();
  auto &registry = SharedObjectRegistry::shared();
  auto id = native->sharedObjectId();
  if (id == 0 || registry.find(id) != native) id = registry.add(native);
  object.setNativeState(runtime, std::make_shared<::expo::SharedObject::NativeState>(id, releaseSharedObject));
  objects_.insert_or_assign(id, facebook::jsi::WeakObject(runtime, object));
}

std::optional<Object> RuntimeState::liveObject(Runtime &runtime, SharedObjectId id) {
  forgetReleased();
  auto found = objects_.find(id);
  if (found == objects_.end()) return std::nullopt;
  auto value = found->second.lock(runtime);
  if (!value.isObject()) {
    objects_.erase(found);
    return std::nullopt;
  }
  return value.getObject(runtime);
}

Value RuntimeState::objectFor(Runtime &runtime, const std::shared_ptr<SharedObject> &native) {
  if (const auto id = native->sharedObjectId(); id != 0) {
    if (auto live = liveObject(runtime, id)) return Value(runtime, *live);
  }
  const SharedObject &instance = *native;
  auto klass = classes_.find(std::type_index(typeid(instance)));
  if (klass == classes_.end()) throw JSError(runtime, "This native object has no JavaScript class");
  auto prototype = klass->second.getPropertyAsObject(runtime, "prototype");
  auto object = ::expo::common::createObjectWithPrototype(runtime, &prototype);
  pair(runtime, object, native);
  return Value(runtime, object);
}

RuntimeState::CreatedPromise RuntimeState::createPromise(Runtime &runtime) {
  const auto id = nextPromiseId_++;
  auto pending = std::make_shared<std::optional<PendingPromise>>();
  auto executor = Function::createFromHostFunction(
      runtime,
      PropNameID::forAscii(runtime, "executor"),
      2,
      [pending](Runtime &rt, const Value &, const Value *arguments, size_t count) {
        if (count >= 2) {
          pending->emplace(PendingPromise{
              .resolve = arguments[0].getObject(rt).getFunction(rt),
              .reject = arguments[1].getObject(rt).getFunction(rt),
          });
        }
        return Value::undefined();
      });
  auto promise = runtime.global().getPropertyAsFunction(runtime, "Promise").callAsConstructor(runtime, executor);
  if (pending->has_value()) promises_.try_emplace(id, std::move(**pending));
  return {.promise = std::move(promise), .id = id};
}

void RuntimeState::settle(Runtime &runtime, std::uint64_t id, const PromiseOutcome &outcome) {
  auto node = promises_.extract(id);
  if (node.empty()) return;
  const auto &promise = node.mapped();
  if (outcome.error) {
    promise.reject.call(runtime, makeError(runtime, *outcome.error, outcome.code));
    return;
  }
  Value value;
  try {
    value = outcome.value ? outcome.value(runtime) : Value::undefined();
  } catch (const JSError &error) {
    promise.reject.call(runtime, Value(runtime, error.value()));
    return;
  } catch (const std::exception &error) {
    promise.reject.call(runtime, makeError(runtime, error.what(), ""));
    return;
  }
  promise.resolve.call(runtime, value);
}

void RuntimeState::forgetReleased() {
  for (const auto id : SharedObjectRegistry::shared().takeReleased()) objects_.erase(id);
}

} // namespace rntizen::expo
