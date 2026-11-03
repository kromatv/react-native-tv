#include "ComponentViewRegistry.h"

#include "../views/ComponentView.h"

#include <doctest/doctest.h>

#include <Ecore_Evas.h>

namespace rntizen {

TEST_CASE("createComponentView builds a registered component with its factory, any other as a plain view") {
  static const bool initialized = ecore_evas_init() > 0;
  REQUIRE(initialized);
  auto *canvas = ecore_evas_buffer_new(100, 100);
  auto *evas = ecore_evas_get(canvas);
  registerComponentView("RegistryTestView", [](Evas *on, facebook::react::Tag tag) {
    return std::make_unique<ComponentView>(on, tag + 1000);
  });
  facebook::react::ShadowView registered{};
  registered.componentName = "RegistryTestView";
  registered.tag = 7;
  facebook::react::ShadowView unknown{};
  unknown.componentName = "UnknownTestView";
  unknown.tag = 8;

  const auto first = createComponentView(evas, registered);
  const auto second = createComponentView(evas, unknown);

  CHECK(first->tag() == 1007);
  CHECK(second->tag() == 8);
  ecore_evas_free(canvas);
}

} // namespace rntizen
