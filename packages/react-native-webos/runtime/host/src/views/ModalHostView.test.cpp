#include "ModalHostView.h"

#include "../render/RasterScene.test.h"
#include "ViewFixtures.test.h"

#include <doctest/doctest.h>

namespace rnwebos {

namespace {

using testing::filled;
using testing::frameOf;
using testing::nearColor;
using testing::place;
using testing::propsWith;

} // namespace

TEST_CASE("a Modal is drawn over everything from the screen's corner, wherever it is declared") {
  testing::RasterScene frame(PixelSize{200, 100});
  const auto root = testing::rootOf(frame.scene());
  ComponentView page(frame.scene(), 2);
  ComponentView corner(frame.scene(), 3);
  ModalHostView modal(frame.scene(), 4);
  ComponentView dialog(frame.scene(), 5);
  place(page, filled(255, 0, 0), frameOf(0, 0, 200, 100));
  place(
      corner,
      propsWith([](facebook::react::ViewProps &props) {
        props.yogaStyle.setOverflow(facebook::yoga::Overflow::Hidden);
        props.opacity = 0.5;
      }),
      frameOf(150, 60, 10, 10));
  place(modal, std::make_shared<facebook::react::ViewProps>(), frameOf(150, 60, 200, 100));
  place(dialog, filled(0, 0, 255), frameOf(20, 20, 60, 40));
  root->mountChild(page, 0);
  root->mountChild(corner, 1);
  corner.mountChild(modal, 0);
  modal.mountChild(dialog, 0);

  frame.draw();

  CHECK(nearColor(frame.pixel(40, 40), SK_ColorBLUE));
  CHECK(nearColor(frame.pixel(120, 40), SK_ColorRED));

  corner.unmountChild(modal);
  frame.draw();
  CHECK(nearColor(frame.pixel(40, 40), SK_ColorRED));
}

TEST_CASE("Back goes to the Modal mounted last in the scene, and to none once they are gone") {
  Scene scene(PixelSize{200, 100});
  Scene other(PixelSize{200, 100});
  ComponentView page(scene, 1);
  CHECK_FALSE(ModalHostView::requestCloseTopmost(scene));

  {
    ModalHostView first(scene, 2);
    ModalHostView second(scene, 3);
    page.mountChild(first, 0);
    page.mountChild(second, 1);

    CHECK(ModalHostView::requestCloseTopmost(scene));
    CHECK_FALSE(ModalHostView::requestCloseTopmost(other));

    page.unmountChild(second);
    CHECK(ModalHostView::requestCloseTopmost(scene));
    page.unmountChild(first);
    CHECK_FALSE(ModalHostView::requestCloseTopmost(scene));

    page.mountChild(first, 0);
    CHECK(ModalHostView::requestCloseTopmost(scene));
  }

  CHECK_FALSE(ModalHostView::requestCloseTopmost(scene));
}

} // namespace rnwebos
