#pragma once

#include "RuntimeState.h"
#include "SharedObject.h"

#include "modules/ModuleContext.h"

#include <ReactCommon/CallInvoker.h>
#include <folly/dynamic.h>
#include <jsi/jsi.h>

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace rntizen::expo {

class Module;

/// What Expo modules share in one app: the way back to the JavaScript thread,
/// what the host knows about the app, and the module instances. Thread-safe;
/// the JavaScript side of it is the RuntimeState.
class Context final : public std::enable_shared_from_this<Context> {
 public:
  Context(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, ModuleContext host);

  const ModuleContext &host() const;

  /// The directory the package's resources are in, with a trailing slash.
  const std::string &resourceDirectory() const;

  /// Runs `task` on the JavaScript thread, through React Native's scheduler, so
  /// promise reactions it causes run right after it.
  void runOnJs(std::function<void(facebook::jsi::Runtime &runtime)> task) const;

  void settlePromise(std::uint64_t id, PromiseOutcome outcome) const;

  /// Emits `event` on the JavaScript object of the module `moduleName`, from any thread.
  void emit(const std::string &moduleName, const std::string &event, JsValueFactory payload) const;

  /// Emits `event` on the JavaScript object paired with `object`, from any thread.
  void emit(const std::shared_ptr<SharedObject> &object, const std::string &event, JsValueFactory payload) const;

  /// The JavaScript object of the module `name`, built from its definition;
  /// nullopt when no module has that name. JavaScript thread only.
  std::optional<facebook::jsi::Object> createModuleObject(facebook::jsi::Runtime &runtime, const std::string &name);

 private:
  const std::shared_ptr<facebook::react::CallInvoker> jsInvoker_;
  const ModuleContext host_;
  const std::string resourceDirectory_;
  std::mutex mutex_;
  std::map<std::string, std::shared_ptr<Module>, std::less<>> modules_;
};

/// `payload` as a JavaScript value, for Context::emit.
JsValueFactory dynamicPayload(folly::dynamic payload);

} // namespace rntizen::expo
