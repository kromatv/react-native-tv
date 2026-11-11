#pragma once

#include "../registry/Registration.h"

#include <react/renderer/componentregistry/ComponentDescriptorFactory.h>
#include <react/renderer/componentregistry/ComponentDescriptorProvider.h>

#include <string>

namespace rntizen {

void registerComponentDescriptor(facebook::react::ComponentDescriptorProvider provider);

/// Fabric's component registry over every registered descriptor. A component no
/// descriptor answers lays out as an unimplemented view, so an unsupported native
/// component shows up empty rather than crashing.
facebook::react::ComponentRegistryFactory componentRegistryFactory();

/// Whether a native component exists for `name`, a JavaScript view name such as
/// `RCTView` or a Fabric component name such as `View`.
bool hasComponent(const std::string &name);

template <typename Descriptor>
bool registerComponentDescriptorType() {
  registerComponentDescriptor(facebook::react::concreteComponentDescriptorProvider<Descriptor>());
  return true;
}

} // namespace rntizen

/// Lets Fabric create and lay out the component `Type` describes.
#define RN_TIZEN_COMPONENT_DESCRIPTOR(Type) RN_TIZEN_REGISTRATION(::rntizen::registerComponentDescriptorType<Type>())
