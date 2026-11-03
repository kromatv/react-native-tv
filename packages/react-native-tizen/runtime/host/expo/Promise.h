#pragma once

#include "Context.h"
#include "Convert.h"
#include "RuntimeState.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

namespace rntizen::expo {

/// The promise an asynchronous function settles itself, later and from any
/// thread: Expo's `Promise` argument. An async function whose last parameter
/// is a Promise gets one and is not settled when its body returns. The first
/// settle wins.
class Promise {
 public:
  Promise(std::shared_ptr<Context> context, std::uint64_t id)
      : context_(std::move(context)), id_(id), settled_(std::make_shared<std::atomic<bool>>(false)) {}

  template <typename T>
  void resolve(T value) const {
    auto shared = std::make_shared<T>(std::move(value));
    settle({.value = [shared](facebook::jsi::Runtime &runtime) { return Convert<T>::toJs(runtime, *shared); }});
  }

  void resolve() const {
    settle({});
  }

  void reject(std::string code, std::string message) const {
    settle({.value = nullptr, .error = std::move(message), .code = std::move(code)});
  }

 private:
  void settle(PromiseOutcome outcome) const {
    if (settled_->exchange(true)) return;
    context_->settlePromise(id_, std::move(outcome));
  }

  std::shared_ptr<Context> context_;
  std::uint64_t id_;
  std::shared_ptr<std::atomic<bool>> settled_;
};

} // namespace rntizen::expo
