#include "ViewBox.h"

#include "EvasColor.h"
#include "NinePatchImage.h"

#include <algorithm>

namespace rntizen {

namespace {

Evas_Object *addRectangleMember(Evas *evas, Evas_Object *smart) {
  auto *rectangle = evas_object_rectangle_add(evas);
  evas_object_pass_events_set(rectangle, EINA_TRUE);
  evas_object_smart_member_add(rectangle, smart);
  evas_object_show(rectangle);
  return rectangle;
}

Evas_Object *addShape(Evas *evas, Evas_Object *smart, bool rounded) {
  return rounded ? addPatchImage(evas, smart) : addRectangleMember(evas, smart);
}

} // namespace

ViewBox::ViewBox(Evas *evas, Evas_Object *smart)
    : evas_(evas), smart_(smart), shadows_(evas, smart), border_(evas, smart), gradients_(evas, smart) {}

ViewBox::~ViewBox() {
  if (clipper_ != nullptr) evas_object_del(clipper_);
  if (background_ != nullptr) evas_object_del(background_);
}

bool ViewBox::update(const BoxStyle &style) {
  if (updated_ && style == style_) return false;
  bool changed = updateClipper(style);
  changed = updateBackground(style) || changed;
  changed = shadows_.update(style.shadows, style.corners) || changed;
  changed = border_.update(style) || changed;
  changed = gradients_.update(style) || changed;
  auto *clip = clipper_ != nullptr ? clipper_ : evas_object_smart_clipped_clipper_get(smart_);
  if (background_ != nullptr) evas_object_clip_set(background_, clip);
  for (auto *gradient : gradients_.objects()) evas_object_clip_set(gradient, clip);
  style_ = style;
  updated_ = true;
  return changed;
}

bool ViewBox::updateBackground(const BoxStyle &style) {
  // Evas draws a clipper that clips nothing as a white rectangle: a box that
  // clips keeps a background, transparent or not, hung from its clipper.
  if (!isVisible(style.backgroundColor) && clipper_ == nullptr) {
    if (background_ == nullptr) return false;
    evas_object_del(background_);
    background_ = nullptr;
    return true;
  }
  const bool rounded = !style.corners.isSquare() && isVisible(style.backgroundColor);
  const bool replaced = background_ == nullptr || backgroundIsImage_ != rounded;
  if (replaced) {
    if (background_ != nullptr) evas_object_del(background_);
    background_ = addShape(evas_, smart_, rounded);
    backgroundIsImage_ = rounded;
  }
  if (rounded && (replaced || style.corners != style_.corners)) setNinePatch(background_, filledPatch(style.corners));
  setColor(background_, style.backgroundColor);
  return replaced;
}

bool ViewBox::updateClipper(const BoxStyle &style) {
  if (!style.clips) {
    if (clipper_ == nullptr) return false;
    evas_object_del(clipper_);
    clipper_ = nullptr;
    return true;
  }
  const bool rounded = !style.corners.isSquare();
  const bool replaced = clipper_ == nullptr || clipperIsImage_ != rounded;
  if (replaced) {
    if (clipper_ != nullptr) evas_object_del(clipper_);
    clipper_ = addShape(evas_, smart_, rounded);
    clipperIsImage_ = rounded;
  }
  if (rounded && (replaced || style.corners != style_.corners)) setNinePatch(clipper_, filledPatch(style.corners));
  return replaced;
}

void ViewBox::layout(const PixelRect &box) {
  if (background_ != nullptr) setGeometry(background_, box);
  if (clipper_ != nullptr) setGeometry(clipper_, box);
  shadows_.layout(box);
  border_.layout(box);
  gradients_.layout(box);
}

Evas_Object *ViewBox::clipper() const {
  return clipper_;
}

std::vector<Evas_Object *> ViewBox::objectsBelowContent() const {
  auto objects = shadows_.objects();
  if (background_ != nullptr) objects.push_back(background_);
  const auto &gradients = gradients_.objects();
  objects.insert(objects.end(), gradients.begin(), gradients.end());
  return objects;
}

Evas_Object *ViewBox::topBelowContent() const {
  if (const auto &gradients = gradients_.objects(); !gradients.empty()) return gradients.back();
  if (background_ != nullptr) return background_;
  const auto &shadows = shadows_.objects();
  return shadows.empty() ? nullptr : shadows.back();
}

std::vector<Evas_Object *> ViewBox::objectsAboveChildren() const {
  return border_.objects();
}

Edges ViewBox::overflow() const {
  auto edges = shadows_.overflow();
  const auto outline = std::max(0, border_.overflow());
  return {
      .left = std::max(edges.left, outline),
      .top = std::max(edges.top, outline),
      .right = std::max(edges.right, outline),
      .bottom = std::max(edges.bottom, outline),
  };
}

} // namespace rntizen
