#include "ComponentViewRegistry.h"

#include "../views/ComponentView.h"
#include "../views/ModalHostView.h"

#include <doctest/doctest.h>

namespace rnwebos {

namespace {

facebook::react::ShadowView shadowView(const char *componentName, facebook::react::Tag tag) {
  facebook::react::ShadowView view{};
  view.componentName = componentName;
  view.tag = tag;
  return view;
}

} // namespace

TEST_CASE("createComponentView builds a registered component with its factory, any other as a plain view") {
  Scene scene(PixelSize{100, 100});
  registerComponentView("RegistryTestView", [](Scene &in, facebook::react::Tag tag) {
    return std::make_unique<ComponentView>(in, tag + 1000);
  });

  const auto first = createComponentView(scene, shadowView("RegistryTestView", 7));
  const auto second = createComponentView(scene, shadowView("UnknownTestView", 8));

  CHECK(first->tag() == 1007);
  CHECK(second->tag() == 8);
}

TEST_CASE("a view registered from its own file with RN_WEBOS_COMPONENT_VIEW is what its component mounts as") {
  Scene scene(PixelSize{100, 100});

  const auto view = createComponentView(scene, shadowView("ModalHostView", 9));

  CHECK(dynamic_cast<ModalHostView *>(view.get()) != nullptr);
  CHECK(view->tag() == 9);
}

} // namespace rnwebos
