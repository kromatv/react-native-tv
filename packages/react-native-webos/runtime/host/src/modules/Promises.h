#pragma once

#include <jsi/jsi.h>

#include <string>

namespace rntizen {

/// `Promise.resolve(value)` in `runtime`.
facebook::jsi::Value resolvedPromise(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &value);

/// `Promise.reject(new Error(message))` in `runtime`.
facebook::jsi::Value rejectedPromise(facebook::jsi::Runtime &runtime, const std::string &message);

/// `Promise.reject(reason)` in `runtime`, for an error JavaScript already threw.
facebook::jsi::Value rejectedPromiseWith(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &reason);

} // namespace rntizen
