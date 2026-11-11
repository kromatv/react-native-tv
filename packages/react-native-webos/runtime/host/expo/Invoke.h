#pragma once

#include "Context.h"
#include "Convert.h"
#include "Promise.h"
#include "RuntimeState.h"
#include "WorkQueue.h"

#include "threading/MainThread.h"

#include <jsi/jsi.h>

#include <cstdint>
#include <memory>
#include <span>
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

const facebook::jsi::Value &argumentAt(std::span<const facebook::jsi::Value> arguments, size_t index);

facebook::jsi::Value errorFor(facebook::jsi::Runtime &runtime, const std::exception &error);

PromiseOutcome failureFor(const std::exception &error);

template <typename Tuple, std::size_t Offset, std::size_t... Index>
auto convertArguments(
    facebook::jsi::Runtime &runtime,
    std::span<const facebook::jsi::Value> arguments,
    std::index_sequence<Index...>) {
  return std::tuple<Plain<std::tuple_element_t<Index + Offset, Tuple>>...>{
      Convert<Plain<std::tuple_element_t<Index + Offset, Tuple>>>::fromJs(runtime, argumentAt(arguments, Index))...};
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
  static decltype(auto) call(F &body, const Handle &self, Tuple &&arguments) {
    return std::apply(
        [&]<typename... Values>(Values &&...values) -> decltype(auto) {
          return body(*self, std::forward<Values>(values)...);
        },
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
  static decltype(auto) call(F &body, const Handle &, Tuple &&arguments) {
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
auto convertedArguments(facebook::jsi::Runtime &runtime, std::span<const facebook::jsi::Value> arguments) {
  using Arguments = typename Signature<F>::Arguments;
  constexpr auto offset = Receiver<Self>::offset;
  constexpr auto converted = std::tuple_size_v<Arguments> - offset - (takesPromise<Arguments>() ? 1 : 0);
  return convertArguments<Arguments, offset>(runtime, arguments, std::make_index_sequence<converted>{});
}

template <typename Self, typename F>
facebook::jsi::Value callSync(
    F &body,
    facebook::jsi::Runtime &runtime,
    const facebook::jsi::Value &thisValue,
    std::span<const facebook::jsi::Value> arguments) {
  using Result = typename Signature<F>::Result;
  const auto self = Receiver<Self>::resolve(runtime, thisValue);
  auto converted = convertedArguments<Self, F>(runtime, arguments);
  if constexpr (std::is_void_v<Result>) {
    Receiver<Self>::call(body, self, std::move(converted));
    return facebook::jsi::Value::undefined();
  } else {
    return Convert<Plain<Result>>::toJs(runtime, Receiver<Self>::call(body, self, std::move(converted)));
  }
}

template <typename Self, typename F>
facebook::jsi::HostFunctionType syncFunction(F body) {
  return [body = std::move(body)](
             facebook::jsi::Runtime &runtime,
             const facebook::jsi::Value &thisValue,
             const facebook::jsi::Value *arguments,
             size_t count) mutable {
    try {
      return callSync<Self>(body, runtime, thisValue, std::span(arguments, count));
    } catch (const facebook::jsi::JSIException &) {
      throw;
    } catch (const std::exception &error) {
      throw facebook::jsi::JSError(runtime, errorFor(runtime, error));
    }
  };
}

template <typename Self, typename F, typename Tuple>
PromiseOutcome outcomeOf(F &body, const typename Receiver<Self>::Handle &self, Tuple &&arguments) {
  using Result = typename Signature<F>::Result;
  if constexpr (std::is_void_v<Result>) {
    Receiver<Self>::call(body, self, std::forward<Tuple>(arguments));
    return {};
  } else {
    auto result = std::make_shared<Plain<Result>>(Receiver<Self>::call(body, self, std::forward<Tuple>(arguments)));
    return {
        .value = [result](facebook::jsi::Runtime &runtime) { return Convert<Plain<Result>>::toJs(runtime, *result); },
        .error = std::nullopt,
        .code = "",
    };
  }
}

template <typename Self, typename F, typename Tuple>
void settleAsync(
    const std::shared_ptr<Context> &context,
    F &body,
    const typename Receiver<Self>::Handle &self,
    Tuple &arguments,
    std::uint64_t id) {
  if constexpr (takesPromise<typename Signature<F>::Arguments>()) {
    const Promise promise(context, id);
    try {
      Receiver<Self>::call(body, self, std::tuple_cat(std::move(arguments), std::make_tuple(promise)));
    } catch (const std::exception &error) {
      const auto failure = failureFor(error);
      promise.reject(failure.code, failure.error.value_or(""));
    }
  } else {
    PromiseOutcome outcome;
    try {
      outcome = outcomeOf<Self>(body, self, std::move(arguments));
    } catch (const std::exception &error) {
      outcome = failureFor(error);
    }
    context->settlePromise(id, std::move(outcome));
  }
}

template <typename F>
struct AsyncBody {
  std::shared_ptr<Context> context;
  std::shared_ptr<F> body;
  Queue queue;
};

template <typename Self, typename F>
void dispatchAsync(
    const AsyncBody<F> &async,
    facebook::jsi::Runtime &runtime,
    const facebook::jsi::Value &thisValue,
    std::span<const facebook::jsi::Value> arguments,
    std::uint64_t id) {
  auto self = Receiver<Self>::resolve(runtime, thisValue);
  auto converted = convertedArguments<Self, F>(runtime, arguments);
  auto shared = std::make_shared<decltype(converted)>(std::move(converted));
  auto task = [context = async.context, body = async.body, self = std::move(self), shared, id] {
    settleAsync<Self>(context, *body, self, *shared, id);
  };
  if (async.queue == Queue::Main) {
    runOnMainThread(std::move(task));
  } else {
    runInBackground(std::move(task));
  }
}

template <typename Self, typename F>
facebook::jsi::Value startAsync(
    const AsyncBody<F> &async,
    facebook::jsi::Runtime &runtime,
    const facebook::jsi::Value &thisValue,
    std::span<const facebook::jsi::Value> arguments) {
  auto &state = RuntimeState::get(runtime);
  auto created = state.createPromise(runtime);
  try {
    dispatchAsync<Self>(async, runtime, thisValue, arguments, created.id);
  } catch (const facebook::jsi::JSError &error) {
    state.settle(runtime, created.id, {.value = nullptr, .error = error.getMessage(), .code = ""});
  } catch (const std::exception &error) {
    state.settle(runtime, created.id, failureFor(error));
  }
  return std::move(created.promise);
}

template <typename Self, typename F>
facebook::jsi::HostFunctionType asyncFunction(std::shared_ptr<Context> context, F body, Queue queue) {
  return [async = AsyncBody<F>{.context = std::move(context), .body = std::make_shared<F>(std::move(body)), .queue = queue}](
             facebook::jsi::Runtime &runtime,
             const facebook::jsi::Value &thisValue,
             const facebook::jsi::Value *arguments,
             size_t count) { return startAsync<Self>(async, runtime, thisValue, std::span(arguments, count)); };
}

} // namespace detail

} // namespace rntizen::expo
