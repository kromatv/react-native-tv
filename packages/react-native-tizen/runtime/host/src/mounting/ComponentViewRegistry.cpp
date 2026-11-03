#include "ComponentViewRegistry.h"

#include <mutex>
#include <unordered_map>

namespace rntizen {

namespace {

struct Registry {
  std::mutex mutex;
  std::unordered_map<std::string, ComponentViewFactory> factories;
};

Registry &registry() {
  static Registry shared;
  return shared;
}

} // namespace

void registerComponentView(std::string componentName, ComponentViewFactory factory) {
  auto &shared = registry();
  std::lock_guard lock(shared.mutex);
  shared.factories.insert_or_assign(std::move(componentName), std::move(factory));
}

std::unique_ptr<ComponentView> createComponentView(Evas *evas, const facebook::react::ShadowView &shadowView) {
  auto &shared = registry();
  std::unique_lock lock(shared.mutex);
  auto found = shared.factories.find(shadowView.componentName);
  if (found == shared.factories.end()) {
    lock.unlock();
    return std::make_unique<ComponentView>(evas, shadowView.tag);
  }
  auto factory = found->second;
  lock.unlock();
  return factory(evas, shadowView.tag);
}

} // namespace rntizen
