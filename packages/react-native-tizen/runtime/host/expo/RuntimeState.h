#pragma once

#include "SharedObject.h"

#include <jsi/jsi.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace rntizen::expo {

class Context;

/// Builds a JavaScript value on the JavaScript thread from native data
/// captured on any thread.
using JsValueFactory = std::function<facebook::jsi::Value(facebook::jsi::Runtime &runtime)>;

/// How an asynchronous native call ended: a value to resolve with, or an error.
struct PromiseOutcome {
  JsValueFactory value;
  std::optional<std::string> error;
  std::string code;
};

/// `new Error(message)` with Expo's `code` property when `code` is not empty.
facebook::jsi::Value makeError(facebook::jsi::Runtime &runtime, const std::string &message, const std::string &code);

/// What Expo keeps per JavaScript runtime and only touches on its thread: the
/// module objects, the JavaScript objects paired with shared objects, the
/// native classes and the promises still waiting for native code. It lives
/// in the runtime's data, so it goes away with the runtime.
class RuntimeState final {
 public:
  explicit RuntimeState(std::shared_ptr<Context> context);

  static void install(facebook::jsi::Runtime &runtime, std::shared_ptr<Context> context);

  /// The state installed in `runtime`; throws a JavaScript error when Expo is not installed.
  static RuntimeState &get(facebook::jsi::Runtime &runtime);

  const std::shared_ptr<Context> &context() const;

  facebook::jsi::Value module(facebook::jsi::Runtime &runtime, const std::string &name);
  std::optional<facebook::jsi::Object> loadedModule(facebook::jsi::Runtime &runtime, const std::string &name);

  void registerClass(facebook::jsi::Runtime &runtime, std::type_index type, const facebook::jsi::Function &klass);

  void pair(facebook::jsi::Runtime &runtime, const facebook::jsi::Object &object, std::shared_ptr<SharedObject> native);

  facebook::jsi::Value objectFor(facebook::jsi::Runtime &runtime, const std::shared_ptr<SharedObject> &native);
  std::optional<facebook::jsi::Object> liveObject(facebook::jsi::Runtime &runtime, SharedObjectId id);

  struct CreatedPromise {
    facebook::jsi::Value promise;
    std::uint64_t id;
  };
  CreatedPromise createPromise(facebook::jsi::Runtime &runtime);
  void settle(facebook::jsi::Runtime &runtime, std::uint64_t id, const PromiseOutcome &outcome);

 private:
  struct PendingPromise {
    facebook::jsi::Function resolve;
    facebook::jsi::Function reject;
  };

  void forgetReleased();

  const std::shared_ptr<Context> context_;
  std::unordered_map<std::string, facebook::jsi::Object> modules_;
  std::unordered_map<std::type_index, facebook::jsi::Function> classes_;
  std::unordered_map<SharedObjectId, facebook::jsi::WeakObject> objects_;
  std::unordered_map<std::uint64_t, PendingPromise> promises_;
  std::uint64_t nextPromiseId_{1};
};

} // namespace rntizen::expo
