#include "ViewBox.h"

#include "EvasColor.h"

#include <algorithm>

namespace rntizen {

ViewBox::ViewBox(Evas *evas, Evas_Object *smart)
    : smart_(smart),
      shadows_(evas, smart),
      border_(evas, smart),
      gradients_(evas, smart),
      background_(evas, smart),
      clipper_(evas, smart) {}

bool ViewBox::update(const BoxStyle &style) {
  if (updated_ && style == style_) return false;
  bool changed = style.clips ? clipper_.show(style.corners) : clipper_.remove();
  changed = updateBackground(style) || changed;
  changed = shadows_.update(style.shadows, style.corners) || changed;
  changed = border_.update(style) || changed;
  changed = gradients_.update(style) || changed;
  auto *clip = clipper_.object() != nullptr ? clipper_.object() : evas_object_smart_clipped_clipper_get(smart_);
  if (auto *background = background_.object()) evas_object_clip_set(background, clip);
  for (auto *gradient : gradients_.objects()) evas_object_clip_set(gradient, clip);
  style_ = style;
  updated_ = true;
  return changed;
}

bool ViewBox::updateBackground(const BoxStyle &style) {
  // Evas draws a clipper that clips nothing as a white rectangle: a box that
  // clips keeps a background, transparent or not, hung from its clipper.
  const bool painted = isVisible(style.backgroundColor);
  if (!painted && !style.clips) return background_.remove();
  const bool replaced = background_.show(painted && !style.clips ? style.corners : Corners{});
  setColor(background_.object(), style.backgroundColor);
  return replaced;
}

void ViewBox::layout(const PixelRect &box) {
  for (const auto *shape : {&background_, &clipper_}) {
    if (auto *object = shape->object()) setGeometry(object, box);
  }
  shadows_.layout(box);
  border_.layout(box);
  gradients_.layout(box);
}

Evas_Object *ViewBox::clipper() const {
  return clipper_.object();
}

std::vector<Evas_Object *> ViewBox::objectsBelowContent() const {
  auto objects = shadows_.objects();
  if (auto *background = background_.object()) objects.push_back(background);
  const auto &gradients = gradients_.objects();
  objects.insert(objects.end(), gradients.begin(), gradients.end());
  return objects;
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
