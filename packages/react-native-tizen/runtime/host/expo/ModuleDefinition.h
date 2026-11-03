#pragma once

#include "Invoke.h"

#include <JSIUtils.h>
#include <SharedObject.h>

#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <vector>

namespace rntizen::expo {

class ModuleDefinition;

/// One Expo module implemented in C++. Subclasses describe themselves in
/// `definition()` the way Expo's Swift and Kotlin modules do, and register
/// with RN_TIZEN_EXPO_MODULE. One instance lives as long as the app.
class Module {
 public:
  virtual ~Module() = default;

  /// Called once, on the JavaScript thread, the first time JavaScript asks for the module.
  virtual ModuleDefinition definition() = 0;

 protected:
  Context &context() const {
    return *context_;
  }

  /// Emits `event` on this module's JavaScript object, from any thread.
  void sendEvent(const std::string &event, folly::dynamic payload) const;

  /// Emits `event` on the JavaScript object paired with `object`, from any thread.
  void sendEvent(const std::shared_ptr<SharedObject> &object, const std::string &event, folly::dynamic payload) const;

 private:
  friend class Context;
  Context *context_{nullptr};
  std::string name_;
};

/// Puts one member (a constant, a function, a class...) on a module object or a class prototype.
using MemberInstaller = std::function<
    void(facebook::jsi::Runtime &runtime, const std::shared_ptr<Context> &context, facebook::jsi::Object &target)>;

namespace detail {

void defineGetter(
    facebook::jsi::Runtime &runtime,
    facebook::jsi::Object &target,
    const std::string &name,
    facebook::jsi::HostFunctionType getter);

void defineAccessor(
    facebook::jsi::Runtime &runtime,
    facebook::jsi::Object &target,
    const std::string &name,
    facebook::jsi::HostFunctionType getter,
    facebook::jsi::HostFunctionType setter);

void defineFunction(
    facebook::jsi::Runtime &runtime,
    facebook::jsi::Object &target,
    const std::string &name,
    facebook::jsi::HostFunctionType body);

} // namespace detail

/// A native class: its JavaScript instances are Expo SharedObjects paired with
/// a `T`, and JavaScript may extend it. `T` derives from SharedObject.
///
///   ClassDefinition<File>("FileSystemFile")
///       .constructor([](std::string uri) { return std::make_shared<File>(uri); })
///       .property("exists", [](File &file) { return file.exists(); })
///       .asyncFunction("text", [](File &file) { return file.text(); })
template <typename T>
class ClassDefinition {
  static_assert(std::is_base_of_v<SharedObject, T>, "A native class must derive from SharedObject");

 public:
  explicit ClassDefinition(std::string name) : name_(std::move(name)) {}

  /// `body(arguments...)` returns the `std::shared_ptr<T>` a `new` pairs with.
  template <typename F>
  ClassDefinition &constructor(F body) {
    constructor_ = [body = std::move(body)](
                       facebook::jsi::Runtime &runtime,
                       const facebook::jsi::Value &thisValue,
                       const facebook::jsi::Value *arguments,
                       size_t count) mutable -> facebook::jsi::Value {
      auto converted = detail::convertedArguments<void, F>(runtime, arguments, count);
      std::shared_ptr<T> native = std::apply(body, std::move(converted));
      RuntimeState::get(runtime).pair(runtime, thisValue.getObject(runtime), std::move(native));
      return facebook::jsi::Value(runtime, thisValue);
    };
    return *this;
  }

  /// A prototype getter: `getter(T &)` runs on every read.
  template <typename F>
  ClassDefinition &property(const char *name, F getter) {
    members_.push_back([name = std::string(name), getter](auto &runtime, auto &, auto &target) {
      detail::defineGetter(runtime, target, name, detail::syncFunction<T>(getter));
    });
    return *this;
  }

  /// A prototype accessor: `getter(T &)` runs on every read and `setter(T &, value)` on every assignment.
  template <typename Get, typename Set>
  ClassDefinition &property(const char *name, Get getter, Set setter) {
    members_.push_back([name = std::string(name), getter, setter](auto &runtime, auto &, auto &target) {
      detail::defineAccessor(
          runtime, target, name, detail::syncFunction<T>(getter), detail::syncFunction<T>(setter));
    });
    return *this;
  }

  /// `body(T &, arguments...)` runs on the JavaScript thread.
  template <typename F>
  ClassDefinition &function(const char *name, F body) {
    members_.push_back([name = std::string(name), body](auto &runtime, auto &, auto &target) {
      detail::defineFunction(runtime, target, name, detail::syncFunction<T>(body));
    });
    return *this;
  }

  /// `body(T &, arguments...)` runs on `queue` and settles the promise JavaScript gets.
  template <typename F>
  ClassDefinition &asyncFunction(const char *name, F body, Queue queue = Queue::Background) {
    members_.push_back([name = std::string(name), body, queue](auto &runtime, auto &context, auto &target) {
      detail::defineFunction(runtime, target, name, detail::asyncFunction<T>(context, body, queue));
    });
    return *this;
  }

  MemberInstaller installer() && {
    return [name = std::move(name_), constructor = std::move(constructor_), members = std::move(members_)](
               facebook::jsi::Runtime &runtime, const std::shared_ptr<Context> &context, facebook::jsi::Object &module) {
      auto klass = ::expo::SharedObject::createClass(runtime, name.c_str(), constructor);
      auto prototype = klass.getPropertyAsObject(runtime, "prototype");
      for (const auto &member : members) member(runtime, context, prototype);
      RuntimeState::get(runtime).registerClass(runtime, std::type_index(typeid(T)), klass);
      module.setProperty(runtime, name.c_str(), klass);
    };
  }

 private:
  std::string name_;
  ::expo::common::ClassConstructor constructor_;
  std::vector<MemberInstaller> members_;
};

/// What a module is to JavaScript, in the shape of Expo's module DSL.
///
///   return ModuleDefinition("ExpoKeepAwake")
///       .asyncFunction("activate", [this](std::string tag) { return activate(tag); }, Queue::Main)
///       .function("isActivated", [this] { return !tags_.empty(); });
class ModuleDefinition {
 public:
  explicit ModuleDefinition(std::string name) : name_(std::move(name)) {}

  const std::string &name() const {
    return name_;
  }

  /// A value read once, when the module is created: `getter()` returns it.
  template <typename F>
  ModuleDefinition &constant(const char *name, F getter) {
    members_.push_back([name = std::string(name), getter](auto &runtime, auto &, auto &target) mutable {
      using Result = typename detail::Signature<F>::Result;
      target.setProperty(runtime, name.c_str(), Convert<detail::Plain<Result>>::toJs(runtime, getter()));
    });
    return *this;
  }

  /// A value `getter()` computes on every read.
  template <typename F>
  ModuleDefinition &property(const char *name, F getter) {
    members_.push_back([name = std::string(name), getter](auto &runtime, auto &, auto &target) {
      detail::defineGetter(runtime, target, name, detail::syncFunction<void>(getter));
    });
    return *this;
  }

  /// `body(arguments...)` runs on the JavaScript thread; its result is returned.
  template <typename F>
  ModuleDefinition &function(const char *name, F body) {
    members_.push_back([name = std::string(name), body](auto &runtime, auto &, auto &target) {
      detail::defineFunction(runtime, target, name, detail::syncFunction<void>(body));
    });
    return *this;
  }

  /// `body(arguments...)` runs on `queue`; JavaScript gets a promise of its result.
  template <typename F>
  ModuleDefinition &asyncFunction(const char *name, F body, Queue queue = Queue::Background) {
    members_.push_back([name = std::string(name), body, queue](auto &runtime, auto &context, auto &target) {
      detail::defineFunction(runtime, target, name, detail::asyncFunction<void>(context, body, queue));
    });
    return *this;
  }

  /// Runs when JavaScript adds the first listener for `event`, and when it removes the last.
  ModuleDefinition &onStartObserving(std::function<void(const std::string &event)> body);
  ModuleDefinition &onStopObserving(std::function<void(const std::string &event)> body);

  template <typename T>
  ModuleDefinition &sharedClass(ClassDefinition<T> definition) {
    members_.push_back(std::move(definition).installer());
    return *this;
  }

  void install(facebook::jsi::Runtime &runtime, const std::shared_ptr<Context> &context, facebook::jsi::Object &module)
      const;

 private:
  std::string name_;
  std::vector<MemberInstaller> members_;
};

} // namespace rntizen::expo
