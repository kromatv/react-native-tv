#include "InstanceGate.h"

namespace rntizen {

InstanceGate::InstanceGate(facebook::react::ReactInstance &instance) : instance_(&instance) {}

void InstanceGate::callFunctionOnModule(const std::string &moduleName, const std::string &method, folly::dynamic &&args) {
  std::lock_guard lock(mutex_);
  if (instance_ != nullptr) instance_->callFunctionOnModule(moduleName, method, std::move(args));
}

void InstanceGate::close() {
  std::lock_guard lock(mutex_);
  instance_ = nullptr;
}

} // namespace rntizen
