#include "ComponentDescriptorRegistry.h"

#include <react/renderer/componentregistry/ComponentDescriptorProviderRegistry.h>
#include <react/renderer/componentregistry/componentNameByReactViewName.h>
#include <react/renderer/components/FBReactNativeSpec/ComponentDescriptors.h>

#include <mutex>
#include <set>
#include <vector>

namespace rntizen {

namespace {

struct Registry {
  std::mutex mutex;
  std::vector<facebook::react::ComponentDescriptorProvider> providers;
  std::set<std::string, std::less<>> names;
};

Registry &registry() {
  static Registry shared;
  return shared;
}

std::shared_ptr<const facebook::react::ComponentDescriptorProviderRegistry> providerRegistry() {
  static auto providers = [] {
    auto built = std::make_shared<facebook::react::ComponentDescriptorProviderRegistry>();
    auto &shared = registry();
    std::lock_guard lock(shared.mutex);
    for (const auto &provider : shared.providers) built->add(provider);
    return built;
  }();
  return providers;
}

} // namespace

void registerComponentDescriptor(facebook::react::ComponentDescriptorProvider provider) {
  auto &shared = registry();
  std::lock_guard lock(shared.mutex);
  shared.names.emplace(provider.name);
  shared.providers.push_back(std::move(provider));
}

bool hasComponent(const std::string &name) {
  auto &shared = registry();
  std::lock_guard lock(shared.mutex);
  return shared.names.contains(facebook::react::componentNameByReactViewName(name));
}

facebook::react::ComponentRegistryFactory componentRegistryFactory() {
  using namespace facebook::react;
  return [](const EventDispatcher::Weak &eventDispatcher, const std::shared_ptr<const ContextContainer> &contextContainer) {
    auto descriptors = providerRegistry()->createComponentDescriptorRegistry(
        {.eventDispatcher = eventDispatcher, .contextContainer = contextContainer});
    std::const_pointer_cast<ComponentDescriptorRegistry>(descriptors)->setFallbackComponentDescriptor(
        std::make_shared<UnimplementedNativeViewComponentDescriptor>(
            ComponentDescriptorParameters{.eventDispatcher = eventDispatcher, .contextContainer = contextContainer}));
    return descriptors;
  };
}

} // namespace rntizen
