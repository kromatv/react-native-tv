#pragma once

#include "Context.h"
#include "Convert.h"
#include "Promise.h"
#include "RuntimeState.h"
#include "WorkQueue.h"

#include "threading/MainThread.h"

#include <jsi/jsi.h>

#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace rntizen::expo {

/// Where an asynchronous function's body runs: a background thread by
/// default, as for Expo's AsyncFunction, or the EFL main loop for code that
/// touches Evas or the window.
enum class Queue {
  Background,
  Main,
};

namespace detail {

template <typename F>
struct Signature : Signature<decltype(&F::operator())> {};

template <typename Owner, typename R, typename... A>
struct Signature<R (Owner::*)(A...) const> {
  using Result = R;
  using Arguments = std::tuple<A...>;
};

template <typename Owner, typename R, typename... A>
struct Signature<R (Owner::*)(A...)> : Signature<R (Owner::*)(A...) const> {};

template <typename T>
using Plain = std::remove_cvref_t<T>;

const facebook::jsi::Value &argumentAt(const facebook::jsi::Value *arguments, size_t count, size_t index);

facebook::jsi::Value errorFor(facebook::jsi::Runtime &runtime, const std::exception &error);

PromiseOutcome failureFor(const std::exception &error);

template <typename Tuple, std::size_t Offset, std::size_t... Index>
auto convertArguments(
    facebook::jsi::Runtime &runtime,
    const facebook::jsi::Value *arguments,
    size_t count,
    std::index_sequence<Index...>) {
  return std::tuple<Plain<std::tuple_element_t<Index + Offset, Tuple>>...>{
      Convert<Plain<std::tuple_element_t<Index + Offset, Tuple>>>::fromJs(
          runtime, argumentAt(arguments, count, Index))...};
}

template <typename Self>
struct Receiver {
  static constexpr std::size_t offset = 1;
  using Handle = std::shared_ptr<Self>;

  static Handle resolve(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &thisValue) {
    auto self = std::dynamic_pointer_cast<Self>(sharedObjectFromJs(runtime, thisValue));
    if (!self) throw facebook::jsi::JSError(runtime, "The native object was released or is of another class");
    return self;
  }

  template <typename F, typename Tuple>
  static decltype(auto) call(F &body, Handle &self, Tuple &&arguments) {
    return std::apply(
        [&](auto &&...values) -> decltype(auto) { return body(*self, std::forward<decltype(values)>(values)...); },
        std::forward<Tuple>(arguments));
  }
};

template <>
struct Receiver<void> {
  static constexpr std::size_t offset = 0;
  struct Handle {};

  static Handle resolve(facebook::jsi::Runtime &, const facebook::jsi::Value &) {
    return {};
  }

  template <typename F, typename Tuple>
  static decltype(auto) call(F &body, Handle &, Tuple &&arguments) {
    return std::apply(body, std::forward<Tuple>(arguments));
  }
};

template <typename Arguments>
constexpr bool takesPromise() {
  constexpr auto size = std::tuple_size_v<Arguments>;
  if constexpr (size == 0) {
    return false;
  } else {
    return std::is_same_v<Plain<std::tuple_element_t<size - 1, Arguments>>, Promise>;
  }
}

template <typename Self, typename F>
auto convertedArguments(facebook::jsi::Runtime &runtime, const facebook::jsi::Value *arguments, size_t count) {
  using Arguments = typename Signature<F>::Arguments;
  constexpr auto offset = Receiver<Self>::offset;
  constexpr auto converted = std::tuple_size_v<Arguments> - offset - (takesPromise<Arguments>() ? 1 : 0);
  return convertArguments<Arguments, offset>(runtime, arguments, count, std::make_index_sequence<converted>{});
}

template <typename Self, typename F>
facebook::jsi::HostFunctionType syncFunction(F body) {
  return [body = std::move(body)](
             facebook::jsi::Runtime &runtime,
             const facebook::jsi::Value &thisValue,
             const facebook::jsi::Value *arguments,
             size_t count) mutable -> facebook::jsi::Value {
    using Result = typename Signature<F>::Result;
    try {
      auto self = Receiver<Self>::resolve(runtime, thisValue);
      auto converted = convertedArguments<Self, F>(runtime, arguments, count);
      if constexpr (std::is_void_v<Result>) {
        Receiver<Self>::call(body, self, std::move(converted));
        return facebook::jsi::Value::undefined();
      } else {
        return Convert<Plain<Result>>::toJs(runtime, Receiver<Self>::call(body, self, std::move(converted)));
      }
    } catch (const facebook::jsi::JSIException &) {
      throw;
    } catch (const std::exception &error) {
      throw facebook::jsi::JSError(runtime, errorFor(runtime, error));
    }
  };
}

template <typename Self, typename F>
facebook::jsi::HostFunctionType asyncFunction(std::shared_ptr<Context> context, F body, Queue queue) {
  return [context = std::move(context), body = std::make_shared<F>(std::move(body)), queue](
             facebook::jsi::Runtime &runtime,
             const facebook::jsi::Value &thisValue,
             const facebook::jsi::Value *arguments,
             size_t count) -> facebook::jsi::Value {
    using Result = typename Signature<F>::Result;
    auto &state = RuntimeState::get(runtime);
    auto created = state.createPromise(runtime);
    try {
      auto self = Receiver<Self>::resolve(runtime, thisValue);
      auto converted = std::make_shared<decltype(convertedArguments<Self, F>(runtime, arguments, count))>(
          convertedArguments<Self, F>(runtime, arguments, count));
      auto task = [context, body, self, converted, id = created.id]() mutable {
        if constexpr (takesPromise<typename Signature<F>::Arguments>()) {
          Promise promise(context, id);
          try {
            Receiver<Self>::call(*body, self, std::tuple_cat(std::move(*converted), std::make_tuple(promise)));
          } catch (const std::exception &error) {
            auto failure = failureFor(error);
            promise.reject(failure.code, failure.error.value_or(""));
          }
        } else {
          PromiseOutcome outcome;
          try {
            if constexpr (std::is_void_v<Result>) {
              Receiver<Self>::call(*body, self, std::move(*converted));
            } else {
              auto result = std::make_shared<Plain<Result>>(Receiver<Self>::call(*body, self, std::move(*converted)));
              outcome.value = [result](facebook::jsi::Runtime &rt) { return Convert<Plain<Result>>::toJs(rt, *result); };
            }
          } catch (const std::exception &error) {
            outcome = failureFor(error);
          }
          context->settlePromise(id, std::move(outcome));
        }
      };
      if (queue == Queue::Main) {
        runOnMainThread(std::move(task));
      } else {
        runInBackground(std::move(task));
      }
    } catch (const facebook::jsi::JSError &error) {
      state.settle(runtime, created.id, {.value = nullptr, .error = error.getMessage(), .code = ""});
    } catch (const std::exception &error) {
      state.settle(runtime, created.id, failureFor(error));
    }
    return std::move(created.promise);
  };
}

} // namespace detail

} // namespace rntizen::expo
