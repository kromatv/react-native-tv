#pragma once

#include <folly/dynamic.h>
#include <react/runtime/ReactInstance.h>

#include <mutex>
#include <string>

namespace rntizen {

/// Lets any thread call into a ReactInstance until the host closes the gate.
/// `close` waits for calls in flight, so the instance can be destroyed once it
/// returns, and every later call is dropped.
class InstanceGate final {
 public:
  explicit InstanceGate(facebook::react::ReactInstance &instance);

  InstanceGate(const InstanceGate &) = delete;
  InstanceGate &operator=(const InstanceGate &) = delete;

  /// Queues `moduleName.method(...args)` on the JavaScript thread, as
  /// ReactInstance::callFunctionOnModule does; nothing once closed.
  void callFunctionOnModule(const std::string &moduleName, const std::string &method, folly::dynamic &&args);

  void close();

 private:
  std::mutex mutex_;
  facebook::react::ReactInstance *instance_;
};

} // namespace rntizen
