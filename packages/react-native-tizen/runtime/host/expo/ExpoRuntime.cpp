#include "ExpoRuntime.h"

#include "AppDirectories.h"
#include "ExpoModulesHostObject.h"
#include "RuntimeState.h"
#include "SharedObjectRegistry.h"
#include "Uuid.h"
#include "views/ExpoViewRegistry.h"

#include "logging/Log.h"

#include <EventEmitter.h>
#include <NativeModule.h>
#include <SharedObject.h>
#include <SharedRef.h>
#include <jsi/JSIDynamic.h>

#include <cstdlib>

namespace rntizen::expo {

namespace {

using facebook::jsi::Function;
using facebook::jsi::JSError;
using facebook::jsi::Object;
using facebook::jsi::PropNameID;
using facebook::jsi::Runtime;
using facebook::jsi::String;
using facebook::jsi::Value;

Function hostFunction(Runtime &runtime, const char *name, unsigned int argumentCount, facebook::jsi::HostFunctionType body) {
  return Function::createFromHostFunction(runtime, PropNameID::forAscii(runtime, name), argumentCount, std::move(body));
}

std::string stringArgument(Runtime &runtime, const Value *arguments, size_t count, size_t index, const char *function) {
  if (index >= count || !arguments[index].isString()) {
    throw JSError(runtime, std::string(function) + " expects a string as argument " + std::to_string(index + 1));
  }
  return arguments[index].getString(runtime).utf8(runtime);
}

Object versionObject(Runtime &runtime) {
  const std::string version = EXPO_MODULES_CORE_VERSION;
  int parts[3] = {0, 0, 0};
  std::size_t start = 0;
  for (int &part : parts) {
    if (start > version.size()) break;
    part = std::atoi(version.c_str() + start);
    const auto dot = version.find('.', start);
    start = dot == std::string::npos ? version.size() + 1 : dot + 1;
  }
  Object object(runtime);
  object.setProperty(runtime, "version", String::createFromAscii(runtime, version));
  object.setProperty(runtime, "major", parts[0]);
  object.setProperty(runtime, "minor", parts[1]);
  object.setProperty(runtime, "patch", parts[2]);
  return object;
}

Value viewConfig(Runtime &runtime, const Value *arguments, size_t count) {
  const auto module = stringArgument(runtime, arguments, count, 0, "getViewConfig");
  const auto view = count > 1 && arguments[1].isString() ? arguments[1].getString(runtime).utf8(runtime) : std::string();
  auto config = expoViewConfig(module, view);
  return config ? facebook::jsi::valueFromDynamic(runtime, *config) : Value::null();
}

void installHelpers(Runtime &runtime, Object &expo) {
  const auto &directories = appDirectories();
  expo.setProperty(runtime, "expoModulesCoreVersion", versionObject(runtime));
  expo.setProperty(runtime, "cacheDir", String::createFromUtf8(runtime, fileUri(directories.cache)));
  expo.setProperty(runtime, "documentsDir", String::createFromUtf8(runtime, fileUri(directories.data)));
  expo.setProperty(runtime, "uuidv4", hostFunction(runtime, "uuidv4", 0, [](Runtime &rt, const Value &, const Value *, size_t) {
    return Value(String::createFromAscii(rt, uuidV4()));
  }));
  expo.setProperty(
      runtime, "uuidv5", hostFunction(runtime, "uuidv5", 2, [](Runtime &rt, const Value &, const Value *arguments, size_t count) {
        const auto name = stringArgument(rt, arguments, count, 0, "uuidv5");
        const auto space = stringArgument(rt, arguments, count, 1, "uuidv5");
        auto uuid = uuidV5(name, space);
        if (!uuid) throw JSError(rt, "uuidv5 expects a UUID as its namespace, got '" + space + "'");
        return Value(String::createFromAscii(rt, *uuid));
      }));
  expo.setProperty(
      runtime, "getViewConfig", hostFunction(runtime, "getViewConfig", 2, [](Runtime &rt, const Value &, const Value *arguments, size_t count) {
        return viewConfig(rt, arguments, count);
      }));
  expo.setProperty(
      runtime, "reloadAppAsync", hostFunction(runtime, "reloadAppAsync", 1, [](Runtime &rt, const Value &, const Value *, size_t) {
        auto &state = RuntimeState::get(rt);
        auto created = state.createPromise(rt);
        state.settle(rt, created.id, {.value = nullptr, .error = "reloadAppAsync is not supported on Tizen: the host cannot reload yet", .code = "ERR_UNAVAILABLE"});
        return std::move(created.promise);
      }));
}

void releaseSharedObject(const ::expo::SharedObject::ObjectId id) {
  SharedObjectRegistry::shared().release(id);
}

} // namespace

void installExpoGlobals(Runtime &runtime, std::shared_ptr<Context> context) {
  if (runtime.global().hasProperty(runtime, "expo")) return;
  Object expo(runtime);
  installHelpers(runtime, expo);
  runtime.global().setProperty(runtime, "expo", expo);

  ::expo::EventEmitter::installClass(runtime);
  ::expo::SharedObject::installBaseClass(runtime, releaseSharedObject);
  ::expo::SharedRef::installBaseClass(runtime);
  ::expo::NativeModule::installClass(runtime);

  RuntimeState::install(runtime, std::move(context));
  expo.setProperty(runtime, "modules", Object::createFromHostObject(runtime, std::make_shared<ExpoModulesHostObject>()));
}

void installExpoRuntime(Runtime &runtime) {
  auto proxy = runtime.global().getProperty(runtime, "nativeModuleProxy");
  auto core = proxy.isObject() ? proxy.getObject(runtime).getProperty(runtime, "ExpoModulesCore") : Value::undefined();
  if (!core.isObject()) {
    log(LogLevel::Error, "No ExpoModulesCore TurboModule: Expo modules are not installed");
    return;
  }
  auto module = core.getObject(runtime);
  module.getPropertyAsFunction(runtime, "installModules").callWithThis(runtime, module);
}

} // namespace rntizen::expo
