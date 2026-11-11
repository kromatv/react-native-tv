#pragma once

#include "../registry/Registration.h"
#include "../views/ComponentView.h"

#include <react/renderer/mounting/ShadowView.h>

#include <functional>
#include <memory>
#include <string>

namespace rntizen {

/// Creates the native view of one Fabric component.
using ComponentViewFactory = std::function<std::unique_ptr<ComponentView>(Evas *evas, facebook::react::Tag tag)>;

void registerComponentView(std::string componentName, ComponentViewFactory factory);

/// The view registered for the shadow view's component, or a plain ComponentView.
std::unique_ptr<ComponentView> createComponentView(Evas *evas, const facebook::react::ShadowView &shadowView);

template <typename View>
bool registerComponentViewType(const char *componentName) {
  registerComponentView(componentName, [](Evas *evas, facebook::react::Tag tag) {
    return std::make_unique<View>(evas, tag);
  });
  return true;
}

} // namespace rntizen

/// Mounts the Fabric component `componentName` (such as "Paragraph") as `Type`.
#define RN_TIZEN_COMPONENT_VIEW(componentName, Type) \
  RN_TIZEN_REGISTRATION(::rntizen::registerComponentViewType<Type>(componentName))
