#include "ComponentViewRegistry.h"

#include <mutex>
#include <string_view>
#include <unordered_map>

namespace rnwebos {

namespace {

struct StringHash {
  using is_transparent = void;

  std::size_t operator()(std::string_view text) const {
    return std::hash<std::string_view>{}(text);
  }
};

struct Registry {
  std::mutex mutex;
  std::unordered_map<std::string, ComponentViewFactory, StringHash, std::equal_to<>> factories;
};

Registry &registry() {
  static Registry shared;
  return shared;
}

ComponentViewFactory factoryFor(std::string_view componentName) {
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
