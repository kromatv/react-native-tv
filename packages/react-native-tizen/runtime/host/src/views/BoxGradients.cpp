#include "BoxGradients.h"

#include "GradientImage.h"
#include "SmartMembers.h"

#include <algorithm>

namespace rntizen {

BoxGradients::BoxGradients(Evas *evas, Evas_Object *smart) : evas_(evas), smart_(smart) {}

BoxGradients::~BoxGradients() {
  for (auto *image : images_) evas_object_del(image);
}

bool BoxGradients::update(const BoxStyle &style) {
  const bool countChanged = resizePatchImages(images_, style.backgroundImages.size(), evas_, smart_);
  stale_ = stale_ || countChanged || style.backgroundImages != style_.backgroundImages || style.corners != style_.corners ||
      style.borderWidths != style_.borderWidths || style.pointScale != style_.pointScale;
  style_.backgroundImages = style.backgroundImages;
  style_.corners = style.corners;
  style_.borderWidths = style.borderWidths;
  style_.pointScale = style.pointScale;
  return countChanged;
}

void BoxGradients::layout(const PixelRect &box) {
  if (images_.empty()) return;
  const auto &widths = style_.borderWidths;
  auto area = inflated(box, -widths.left, -widths.top, -widths.right, -widths.bottom);
  area.width = std::max(0, area.width);
  area.height = std::max(0, area.height);
  if (stale_ || area.width != painted_.width || area.height != painted_.height) paint(area);
  for (auto *image : images_) setGeometry(image, area);
}

const std::vector<Evas_Object *> &BoxGradients::objects() const {
  return images_;
}

void BoxGradients::paint(const PixelRect &area) {
  const GradientBox box{
      .width = area.width,
      .height = area.height,
      .pointScale = style_.pointScale,
      .corners = style_.corners.inset(style_.borderWidths),
  };
  const auto &layers = style_.backgroundImages;
  for (std::size_t i = 0; i < images_.size(); ++i) {
    const auto &patch = gradientPatch(layers[layers.size() - 1 - i], box);
    if (patch.pixels.empty()) {
      evas_object_hide(images_[i]);
      continue;
    }
    setNinePatch(images_[i], patch);
    evas_object_show(images_[i]);
  }
  painted_ = area;
  stale_ = false;
}

} // namespace rntizen
