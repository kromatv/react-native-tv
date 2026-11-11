#include "ComponentViewRegistry.h"

#include <mutex>
#include <unordered_map>

namespace rnwebos {

namespace {

struct Registry {
  std::mutex mutex;
  std::unordered_map<std::string, ComponentViewFactory> factories;
};

Registry &registry() {
  static Registry shared;
  return shared;
}

ComponentViewFactory factoryFor(const std::string &componentName) {
  auto &shared = registry();
  std::lock_guard lock(shared.mutex);
  const auto found = shared.factories.find(componentName);
  return found == shared.factories.end() ? ComponentViewFactory{} : found->second;
}

} // namespace

void registerComponentView(std::string componentName, ComponentViewFactory factory) {
  auto &shared = registry();
  std::lock_guard lock(shared.mutex);
  shared.factories.insert_or_assign(std::move(componentName), std::move(factory));
}

std::unique_ptr<ComponentView> createComponentView(Scene &scene, const facebook::react::ShadowView &shadowView) {
  if (const auto factory = factoryFor(shadowView.componentName)) return factory(scene, shadowView.tag);
  return std::make_unique<ComponentView>(scene, shadowView.tag);
}

} // namespace rnwebos
