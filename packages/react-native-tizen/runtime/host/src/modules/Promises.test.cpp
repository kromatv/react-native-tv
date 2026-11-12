#include "Promises.h"

#include "TurboModuleTest.test.h"

#include <doctest/doctest.h>

namespace rntizen {

using testing::settled;

TEST_CASE("resolvedPromise resolves with the value it is given") {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;

  CHECK(settled(rt, resolvedPromise(rt, facebook::jsi::Value(42))) == "resolved: 42");
  CHECK(settled(rt, resolvedPromise(rt, testing::evaluate(rt, "{answer: 'yes'}"))) == R"(resolved: {"answer":"yes"})");
}

TEST_CASE("rejectedPromise rejects with an Error that carries the message") {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  const auto promise = rejectedPromise(rt, "The specified blob is invalid");

  CHECK(settled(rt, promise) == "rejected: The specified blob is invalid");
  rt.global().setProperty(rt, "rejected", promise);
  testing::evaluate(rt, "rejected.catch(error => { globalThis.isError = error instanceof Error; })");
  rt.drainMicrotasks();
  CHECK(rt.global().getProperty(rt, "isError").getBool());
}

TEST_CASE("rejectedPromiseWith rejects with the reason itself, not a new Error") {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  const auto reason = testing::evaluate(rt, "globalThis.reason = new TypeError('thrown by JavaScript')");
  const auto promise = rejectedPromiseWith(rt, reason);

  CHECK(settled(rt, promise) == "rejected: thrown by JavaScript");
  rt.global().setProperty(rt, "rejected", promise);
  testing::evaluate(rt, "rejected.catch(error => { globalThis.same = error === reason; })");
  rt.drainMicrotasks();
  CHECK(rt.global().getProperty(rt, "same").getBool());
}

} // namespace rntizen
