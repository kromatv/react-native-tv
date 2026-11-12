#include "BoxBorder.h"

#include "EvasColor.h"
#include "SmartMembers.h"

#include <algorithm>

namespace rntizen {

namespace {

using facebook::react::SharedColor;

bool hasBorder(const BoxStyle &style) {
  const auto &c = style.borderColors;
  return !style.borderWidths.isZero() &&
      (isVisible(c.left) || isVisible(c.top) || isVisible(c.right) || isVisible(c.bottom));
}

bool hasOneColor(const BoxStyle &style) {
  const auto &c = style.borderColors;
  return c.left == c.top && c.top == c.right && c.right == c.bottom;
}

const SharedColor &ringColor(const BoxStyle &style) {
  const auto &c = style.borderColors;
  for (const auto *color : {&c.top, &c.right, &c.bottom, &c.left}) {
    if (isVisible(*color)) return *color;
  }
  return c.top;
}

void remove(Evas_Object *&object) {
  if (object == nullptr) return;
  evas_object_del(object);
  object = nullptr;
}

} // namespace

BoxBorder::BoxBorder(Evas *evas, Evas_Object *smart) : evas_(evas), smart_(smart) {}

BoxBorder::~BoxBorder() {
  remove(ring_);
  for (auto *&side : sides_) remove(side);
  remove(outline_);
}

bool BoxBorder::update(const BoxStyle &style) {
  const bool border = hasBorder(style);
  const bool ring = border && (hasOneColor(style) || !style.corners.isSquare());
  bool changed = updateRing(style, ring);
  changed = updateSides(style, border && !ring) || changed;
  changed = updateOutline(style) || changed;
  style_ = style;
  return changed;
}

bool BoxBorder::updateRing(const BoxStyle &style, bool wanted) {
  if (!wanted) {
    const bool had = ring_ != nullptr;
    remove(ring_);
    return had;
  }
  const bool created = ring_ == nullptr;
  if (created) ring_ = addPatchImage(evas_, smart_);
  if (created || style.corners != style_.corners || style.borderWidths != style_.borderWidths ||
      style.borderPattern != style_.borderPattern) {
    patterned_ = {};
    if (style.borderPattern == StrokePattern::Solid) setNinePatch(ring_, ringPatch(style.corners, style.borderWidths));
  }
  setColor(ring_, ringColor(style));
  return created;
}

bool BoxBorder::updateSides(const BoxStyle &style, bool wanted) {
  if (!wanted) {
    const bool had = sides_[0] != nullptr;
    for (auto *&side : sides_) remove(side);
    return had;
  }
  const bool created = sides_[0] == nullptr;
  if (created) {
    for (auto *&side : sides_) side = addRectangleMember(evas_, smart_);
  }
  const auto &c = style.borderColors;
  setColor(sides_[0], c.top);
  setColor(sides_[1], c.right);
  setColor(sides_[2], c.bottom);
  setColor(sides_[3], c.left);
  return created;
}

bool BoxBorder::updateOutline(const BoxStyle &style) {
  const auto &outline = style.outline;
  if (outline.width <= 0) {
    const bool had = outline_ != nullptr;
    remove(outline_);
    return had;
  }
  const bool created = outline_ == nullptr;
  if (created) outline_ = addPatchImage(evas_, smart_);
  if (created || style.corners != style_.corners || outline.width != style_.outline.width ||
      outline.offset != style_.outline.offset) {
    const auto width = outline.width;
    setNinePatch(outline_, ringPatch(style.corners.grown(outline.offset + width), {width, width, width, width}));
  }
  setColor(outline_, outline.color);
  return created;
}

void BoxBorder::paintPattern(const PixelRect &box) {
  const auto *patch = patternedRingPatch(style_.corners, style_.borderWidths, box.width, box.height, style_.borderPattern);
  setNinePatch(ring_, patch != nullptr ? *patch : ringPatch(style_.corners, style_.borderWidths));
  patterned_ = box;
}

void BoxBorder::layout(const PixelRect &box) {
  if (ring_ != nullptr) {
    const bool resized = box.width != patterned_.width || box.height != patterned_.height;
    if (style_.borderPattern != StrokePattern::Solid && resized) paintPattern(box);
    setGeometry(ring_, box);
  }
  if (sides_[0] != nullptr) {
    const auto &w = style_.borderWidths;
    const auto middle = std::max(0, box.height - w.top - w.bottom);
    setGeometry(sides_[0], {box.x, box.y, box.width, w.top});
    setGeometry(sides_[1], {box.x + box.width - w.right, box.y + w.top, w.right, middle});
    setGeometry(sides_[2], {box.x, box.y + box.height - w.bottom, box.width, w.bottom});
    setGeometry(sides_[3], {box.x, box.y + w.top, w.left, middle});
  }
  if (outline_ != nullptr) {
    const auto grow = overflow();
    auto rect = inflated(box, grow, grow, grow, grow);
    rect.width = std::max(0, rect.width);
    rect.height = std::max(0, rect.height);
    setGeometry(outline_, rect);
  }
}

std::vector<Evas_Object *> BoxBorder::objects() const {
  std::vector<Evas_Object *> objects;
  if (ring_ != nullptr) objects.push_back(ring_);
  if (sides_[0] != nullptr) objects.insert(objects.end(), sides_.begin(), sides_.end());
  if (outline_ != nullptr) objects.push_back(outline_);
  return objects;
}

int BoxBorder::overflow() const {
  return style_.outline.width > 0 ? style_.outline.offset + style_.outline.width : 0;
}

} // namespace rntizen
