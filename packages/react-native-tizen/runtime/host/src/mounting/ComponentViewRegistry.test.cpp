#include "ComponentViewRegistry.h"

#include "../views/ComponentView.h"
#include "../views/ModalHostView.h"

#include <doctest/doctest.h>

#include <Ecore_Evas.h>

namespace rntizen {

namespace {

class Canvas final {
 public:
  Canvas() {
    static const bool initialized = ecore_evas_init() > 0;
    REQUIRE(initialized);
    canvas_ = ecore_evas_buffer_new(100, 100);
  }
  ~Canvas() {
    ecore_evas_free(canvas_);
  }
  Canvas(const Canvas &) = delete;
  Canvas &operator=(const Canvas &) = delete;

  Evas *evas() const {
    return ecore_evas_get(canvas_);
  }

 private:
  Ecore_Evas *canvas_;
};

facebook::react::ShadowView shadowView(const char *componentName, facebook::react::Tag tag) {
  facebook::react::ShadowView view{};
  view.componentName = componentName;
  view.tag = tag;
  return view;
}

} // namespace

TEST_CASE("createComponentView builds a registered component with its factory, any other as a plain view") {
  const Canvas canvas;
  registerComponentView("RegistryTestView", [](Evas *on, facebook::react::Tag tag) {
    return std::make_unique<ComponentView>(on, tag + 1000);
  });

  const auto first = createComponentView(canvas.evas(), shadowView("RegistryTestView", 7));
  const auto second = createComponentView(canvas.evas(), shadowView("UnknownTestView", 8));

  CHECK(first->tag() == 1007);
  CHECK(second->tag() == 8);
}

TEST_CASE("a view registered from its own file with RN_TIZEN_COMPONENT_VIEW is what its component mounts as") {
  const Canvas canvas;

  const auto view = createComponentView(canvas.evas(), shadowView("ModalHostView", 9));

  CHECK(dynamic_cast<ModalHostView *>(view.get()) != nullptr);
  CHECK(view->tag() == 9);
}

} // namespace rntizen
