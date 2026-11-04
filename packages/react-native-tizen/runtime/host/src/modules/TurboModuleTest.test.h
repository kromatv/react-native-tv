#pragma once

#include "ModuleContext.h"

#include <ReactCommon/CallInvoker.h>
#include <ReactCommon/TurboModule.h>
#include <folly/dynamic.h>
#include <folly/json.h>
#include <hermes/hermes.h>
#include <jsi/JSIDynamic.h>
#include <jsi/jsi.h>

#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace rntizen::testing {

/// A CallInvoker whose tasks wait for the test thread, as the JavaScript thread runs them; any thread may queue.
class QueuedInvoker final : public facebook::react::CallInvoker {
 public:
  void invokeAsync(facebook::react::CallFunc &&task) noexcept override {
    std::lock_guard lock(mutex_);
    tasks_.push_back(std::move(task));
  }

  void invokeSync(facebook::react::CallFunc &&task) override {
    std::lock_guard lock(mutex_);
    tasks_.push_back(std::move(task));
  }

  std::size_t run(facebook::jsi::Runtime &runtime) {
    std::deque<facebook::react::CallFunc> ready;
    {
      std::lock_guard lock(mutex_);
      ready.swap(tasks_);
    }
    for (auto &task : ready) task(runtime);
    return ready.size();
  }

 private:
  std::mutex mutex_;
  std::deque<facebook::react::CallFunc> tasks_;
};

/// The device events a module emitted, from any thread, in order.
class DeviceEvents final {
 public:
  struct Event {
    std::string name;
    folly::dynamic payload;
  };

  std::function<void(const std::string &, folly::dynamic)> emitter() {
    return [this](const std::string &name, folly::dynamic payload) {
      std::lock_guard lock(mutex_);
      events_.push_back({name, std::move(payload)});
      changed_.notify_all();
    };
  }

  /// The events once there are `count` of them, or what arrived in five seconds.
  std::vector<Event> waitFor(std::size_t count) {
    std::unique_lock lock(mutex_);
    changed_.wait_for(lock, std::chrono::seconds(5), [this, count] { return events_.size() >= count; });
    return events_;
  }

  /// The first event named `name`, waiting up to five seconds for it; a null payload when none came.
  folly::dynamic first(const std::string &name) {
    std::unique_lock lock(mutex_);
    folly::dynamic found = nullptr;
    changed_.wait_for(lock, std::chrono::seconds(5), [this, &name, &found] {
      for (const auto &event : events_) {
        if (event.name == name) {
          found = event.payload;
          return true;
        }
      }
      return false;
    });
    return found;
  }

  std::vector<std::string> names() {
    std::lock_guard lock(mutex_);
    std::vector<std::string> list;
    for (const auto &event : events_) list.push_back(event.name);
    return list;
  }

 private:
  std::mutex mutex_;
  std::condition_variable changed_;
  std::vector<Event> events_;
};

inline std::unique_ptr<facebook::jsi::Runtime> makeRuntime() {
  return facebook::hermes::makeHermesRuntime(::hermes::vm::RuntimeConfig::Builder().withMicrotaskQueue(true).build());
}

inline facebook::jsi::Value evaluate(facebook::jsi::Runtime &runtime, const std::string &source) {
  return runtime.evaluateJavaScript(std::make_shared<facebook::jsi::StringBuffer>("(" + source + ")"), "test.js");
}

template <typename... Arguments>
facebook::jsi::Value call(
    facebook::jsi::Runtime &runtime,
    facebook::react::TurboModule &module,
    const char *method,
    Arguments &&...arguments) {
  auto function =
      module.get(runtime, facebook::jsi::PropNameID::forAscii(runtime, method)).asObject(runtime).asFunction(runtime);
  return function.call(runtime, std::forward<Arguments>(arguments)...);
}

inline std::string json(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &value) {
  return folly::toJson(facebook::jsi::dynamicFromValue(runtime, value));
}

/// How `promise` settled once microtasks ran: "resolved: <JSON>" or "rejected: <message>".
inline std::string settled(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &promise) {
  auto outcome = std::make_shared<std::string>("pending");
  auto record = [outcome](const char *prefix, bool message) {
    return [outcome, prefix, message](
               facebook::jsi::Runtime &rt, const facebook::jsi::Value &, const facebook::jsi::Value *arguments, size_t) {
      const auto &value = arguments[0];
      *outcome = std::string(prefix) +
          (message ? value.asObject(rt).getProperty(rt, "message").asString(rt).utf8(rt) : json(rt, value));
      return facebook::jsi::Value::undefined();
    };
  };
  using facebook::jsi::Function;
  using facebook::jsi::PropNameID;
  auto object = promise.asObject(runtime);
  object.getPropertyAsFunction(runtime, "then")
      .callWithThis(
          runtime,
          object,
          Function::createFromHostFunction(runtime, PropNameID::forAscii(runtime, "resolved"), 1, record("resolved: ", false)),
          Function::createFromHostFunction(runtime, PropNameID::forAscii(runtime, "rejected"), 1, record("rejected: ", true)));
  runtime.drainMicrotasks();
  return *outcome;
}

/// The message of the JSError `body` throws: "no exception" when it returns, "not a JSError" for any other exception.
template <typename Body>
std::string jsErrorMessage(Body &&body) {
  try {
    body();
  } catch (const facebook::jsi::JSError &error) {
    return error.getMessage();
  } catch (...) {
    return "not a JSError";
  }
  return "no exception";
}

/// A JavaScript function that keeps what it was last called with, as JSON in `calls`.
inline facebook::jsi::Function recorder(facebook::jsi::Runtime &runtime, std::shared_ptr<std::vector<std::string>> calls) {
  return facebook::jsi::Function::createFromHostFunction(
      runtime,
      facebook::jsi::PropNameID::forAscii(runtime, "recorder"),
      1,
      [calls](facebook::jsi::Runtime &rt, const facebook::jsi::Value &, const facebook::jsi::Value *arguments, size_t count) {
        calls->push_back(count > 0 ? json(rt, arguments[0]) : std::string("undefined"));
        return facebook::jsi::Value::undefined();
      });
}

} // namespace rntizen::testing
