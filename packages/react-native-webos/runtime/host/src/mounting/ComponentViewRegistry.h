#pragma once

#include "../registry/Registration.h"
#include "../views/ComponentView.h"

#include <react/renderer/mounting/ShadowView.h>

#include <functional>
#include <memory>
#include <string>

namespace rnwebos {

/// Creates the native view of one Fabric component.
using ComponentViewFactory = std::function<std::unique_ptr<ComponentView>(Scene &scene, facebook::react::Tag tag)>;

void registerComponentView(std::string componentName, ComponentViewFactory factory);

/// The view registered for the shadow view's component, or a plain ComponentView.
std::unique_ptr<ComponentView> createComponentView(Scene &scene, const facebook::react::ShadowView &shadowView);

template <typename View>
bool registerComponentViewType(const char *componentName) {
  registerComponentView(componentName, [](Scene &scene, facebook::react::Tag tag) {
    return std::make_unique<View>(scene, tag);
  });
  return true;
}

} // namespace rnwebos

/// Mounts the Fabric component `componentName` (such as "Paragraph") as `Type`.
#define RN_WEBOS_COMPONENT_VIEW(componentName, Type) \
  RN_WEBOS_REGISTRATION(::rnwebos::registerComponentViewType<Type>(componentName))
