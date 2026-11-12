#include "InstanceGate.h"

namespace rnwebos {

namespace {

// Android's TRIM_MEMORY_BACKGROUND, the mildest level React Native collects at
// (docs/reference/react-native-platform-contract.md, Memory).
constexpr int kBackgroundMemoryPressure = 40;

} // namespace

InstanceGate::InstanceGate(facebook::react::ReactInstance &instance) : instance_(&instance) {}

void InstanceGate::callFunctionOnModule(const std::string &moduleName, const std::string &method, folly::dynamic &&args) {
  std::lock_guard lock(mutex_);
  if (instance_ != nullptr) instance_->callFunctionOnModule(moduleName, method, std::move(args));
}

void InstanceGate::collectGarbage() {
  std::lock_guard lock(mutex_);
  if (instance_ != nullptr) instance_->handleMemoryPressureJs(kBackgroundMemoryPressure);
}

void InstanceGate::close() {
  std::lock_guard lock(mutex_);
  instance_ = nullptr;
}

} // namespace rnwebos
