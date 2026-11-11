#include "ComponentView.h"

#include "../render/RasterScene.test.h"
#include "ViewFixtures.test.h"

#include <doctest/doctest.h>

#include <memory>
#include <vector>

namespace rnwebos {

namespace {

using facebook::react::Transform;
using testing::filled;
using testing::frameOf;
using testing::place;
using testing::propsWith;

// Counts the frames it is drawn in: a view the scene culls draws no content.
class Probe final : public ComponentView {
 public:
  using ComponentView::ComponentView;

  int draws{0};

 protected:
  void drawContent(SkCanvas &, float) override {
    ++draws;
  }
};

facebook::react::Props::Shared moved(Transform transform) {
  return propsWith([&](facebook::react::ViewProps &props) { props.transform = transform; });
}

facebook::react::Props::Shared clipping() {
  return propsWith([](facebook::react::ViewProps &props) { props.yogaStyle.setOverflow(facebook::yoga::Overflow::Hidden); });
}

struct Stage {
  testing::RasterScene frame{PixelSize{200, 100}};
  std::unique_ptr<ComponentView> root{testing::rootOf(frame.scene())};
};

} // namespace

TEST_CASE("a view is drawn when it shows in the frame and skipped when it is off screen") {
  Stage stage;
  Probe inside(stage.frame.scene(), 2);
  Probe outside(stage.frame.scene(), 3);
  place(inside, filled(255, 0, 0), frameOf(10, 10, 50, 50));
  place(outside, filled(255, 0, 0), frameOf(300, 10, 50, 50));
  stage.root->mountChild(inside, 0);
  stage.root->mountChild(outside, 1);

  stage.frame.draw();

  CHECK(inside.draws == 1);
  CHECK(outside.draws == 0);
}

TEST_CASE("a child translated onto the screen is drawn however far off screen its parent's box is") {
  Stage stage;
  ComponentView parent(stage.frame.scene(), 2);
  Probe child(stage.frame.scene(), 3);
  place(parent, std::make_shared<facebook::react::ViewProps>(), frameOf(-600, 0, 100, 100));
  place(child, moved(Transform::Translate(620, 0, 0)), frameOf(0, 0, 50, 50));
  parent.mountChild(child, 0);
  stage.root->mountChild(parent, 0);

  stage.frame.draw();

  CHECK(child.draws == 1);
}

TEST_CASE("a child scaled up shows where its enlarged box reaches") {
  Stage stage;
  Probe child(stage.frame.scene(), 2);
  place(child, moved(Transform::Scale(5, 5, 1)), frameOf(230, 30, 20, 20));
  stage.root->mountChild(child, 0);

  stage.frame.draw();

  CHECK(child.draws == 1);
}

TEST_CASE("a parent that clips keeps children outside its box undrawn, and off screen it hides them all") {
  Stage stage;
  ComponentView parent(stage.frame.scene(), 2);
  Probe kept(stage.frame.scene(), 3);
  Probe cut(stage.frame.scene(), 4);
  place(parent, clipping(), frameOf(0, 0, 100, 100));
  place(kept, filled(255, 0, 0), frameOf(10, 10, 20, 20));
  place(cut, filled(255, 0, 0), frameOf(120, 10, 20, 20));
  parent.mountChild(kept, 0);
  parent.mountChild(cut, 1);
  stage.root->mountChild(parent, 0);

  stage.frame.draw();
  CHECK(kept.draws == 1);
  CHECK(cut.draws == 0);

  parent.updateLayout(frameOf(-400, 0, 100, 100));
  cut.updateProps(moved(Transform::Translate(300, 0, 0)));
  stage.frame.draw();
  CHECK(kept.draws == 1);
  CHECK(cut.draws == 0);
}

TEST_CASE("what a view covers follows a descendant that moves, grows or leaves") {
  Stage stage;
  ComponentView parent(stage.frame.scene(), 2);
  ComponentView middle(stage.frame.scene(), 3);
  Probe child(stage.frame.scene(), 4);
  place(parent, std::make_shared<facebook::react::ViewProps>(), frameOf(-300, 0, 100, 100));
  place(middle, std::make_shared<facebook::react::ViewProps>(), frameOf(0, 0, 100, 100));
  place(child, filled(255, 0, 0), frameOf(0, 0, 50, 50));
  middle.mountChild(child, 0);
  parent.mountChild(middle, 0);
  stage.root->mountChild(parent, 0);

  stage.frame.draw();
  CHECK(child.draws == 0);

  child.updateLayout(frameOf(310, 0, 50, 50));
  stage.frame.draw();
  CHECK(child.draws == 1);

  child.updateLayout(frameOf(0, 0, 50, 50));
  stage.frame.draw();
  CHECK(child.draws == 1);

  child.updateLayout(frameOf(0, 0, 320, 50));
  stage.frame.draw();
  CHECK(child.draws == 2);

  middle.unmountChild(child);
  Probe late(stage.frame.scene(), 5);
  place(late, filled(255, 0, 0), frameOf(320, 20, 10, 10));
  middle.mountChild(late, 0);
  stage.frame.draw();
  CHECK(late.draws == 1);
}

TEST_CASE("of a long column of rows only those in the frame are drawn") {
  Stage stage;
  ComponentView column(stage.frame.scene(), 2);
  place(column, std::make_shared<facebook::react::ViewProps>(), frameOf(0, -4001, 200, 10000));
  std::vector<std::unique_ptr<Probe>> rows;
  for (int index = 0; index < 500; ++index) {
    rows.push_back(std::make_unique<Probe>(stage.frame.scene(), 10 + index));
    place(*rows.back(), filled(0, 255, 0), frameOf(0, 20 * index, 200, 18));
    column.mountChild(*rows.back(), static_cast<std::size_t>(index));
  }
  stage.root->mountChild(column, 0);

  stage.frame.draw();

  int drawn = 0;
  for (const auto &row : rows) drawn += row->draws;
  CHECK(drawn == 6);
  CHECK(rows[199]->draws == 0);
  CHECK(rows[200]->draws == 1);
  CHECK(rows[205]->draws == 1);
  CHECK(rows[206]->draws == 0);
}

TEST_CASE("a view with opacity and children still shows them where they are") {
  Stage stage;
  ComponentView parent(stage.frame.scene(), 2);
  ComponentView child(stage.frame.scene(), 3);
  place(parent, propsWith([](facebook::react::ViewProps &props) { props.opacity = 0.5; }), frameOf(0, 0, 10, 10));
  place(child, filled(255, 255, 255), frameOf(100, 50, 40, 40));
  parent.mountChild(child, 0);
  stage.root->mountChild(parent, 0);

  stage.frame.draw();

  CHECK(testing::nearColor(stage.frame.pixel(120, 70), SkColorSetRGB(128, 128, 128)));
  CHECK(stage.frame.pixel(90, 70) == SK_ColorBLACK);
}

} // namespace rnwebos
