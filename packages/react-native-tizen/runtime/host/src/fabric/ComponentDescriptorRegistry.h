#pragma once

#include "../registry/Registration.h"

#include <react/renderer/componentregistry/ComponentDescriptorFactory.h>
#include <react/renderer/componentregistry/ComponentDescriptorProvider.h>

#include <string>

namespace rntizen {

void registerComponentDescriptor(facebook::react::ComponentDescriptorProvider provider);

/// Fabric's component registry over every registered descriptor. A component no
/// descriptor answers gets React Native's interop descriptor under its own name and
/// lays out as a plain view; with `useFabricInterop` off, as an unimplemented view.
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
