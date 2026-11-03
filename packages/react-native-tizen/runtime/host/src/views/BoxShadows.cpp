#include "BoxShadows.h"

#include "EvasColor.h"
#include "NinePatchImage.h"
#include "ShadowPatches.h"

#include <algorithm>
#include <utility>

namespace rntizen {

namespace {

int reach(const ShadowStyle &shadow) {
  return shadow.spread + shadowPadding(shadow.blur);
}

} // namespace

BoxShadows::BoxShadows(Evas *evas, Evas_Object *smart)
    : evas_(evas), smart_(smart), alive_(std::make_shared<BoxShadows *>(this)) {}

BoxShadows::~BoxShadows() {
  alive_.reset();
  for (auto *image : images_) evas_object_del(image);
}

bool BoxShadows::update(const std::vector<ShadowStyle> &shadows, const Corners &corners) {
  const bool countChanged = shadows.size() != images_.size();
  while (images_.size() > shadows.size()) {
    evas_object_del(images_.back());
    images_.pop_back();
  }
  while (images_.size() < shadows.size()) images_.push_back(addPatchImage(evas_, smart_));
  for (std::size_t i = 0; i < shadows.size(); ++i) {
    const auto &shadow = shadows[i];
    setColor(images_[i], shadow.color, shadow.opacity);
  }
  const auto previous = std::exchange(shadows_, shadows);
  const auto previousCorners = std::exchange(corners_, corners);
  for (std::size_t i = 0; i < shadows.size(); ++i) {
    const auto &shadow = shadows[i];
    const bool shapeChanged = countChanged || corners != previousCorners || i >= previous.size() ||
        previous[i].blur != shadow.blur || previous[i].spread != shadow.spread;
    if (shapeChanged) showPatch(i, corners.grown(shadow.spread), shadow.blur);
  }
  return countChanged;
}

void BoxShadows::showPatch(std::size_t index, const Corners &corners, int blur) {
  if (const auto *patch = cachedShadowPatch(corners, blur)) {
    setNinePatch(images_[index], *patch);
    return;
  }
  const std::weak_ptr<BoxShadows *> alive = alive_;
  requestShadowPatch(corners, blur, [alive, index, corners, blur](const NinePatch &patch) {
    const auto self = alive.lock();
    if (!self) return;
    auto &shadows = **self;
    if (index >= shadows.images_.size() || index >= shadows.shadows_.size()) return;
    const auto &wanted = shadows.shadows_[index];
    if (wanted.blur != blur || shadows.corners_.grown(wanted.spread) != corners) return;
    setNinePatch(shadows.images_[index], patch);
  });
}

void BoxShadows::layout(const PixelRect &box) {
  for (std::size_t i = 0; i < images_.size(); ++i) {
    const auto &shadow = shadows_[i];
    const auto grow = reach(shadow);
    auto rect = inflated(box, grow, grow, grow, grow);
    rect.x += shadow.offsetX;
    rect.y += shadow.offsetY;
    rect.width = std::max(0, rect.width);
    rect.height = std::max(0, rect.height);
    setGeometry(images_[i], rect);
  }
}

const std::vector<Evas_Object *> &BoxShadows::objects() const {
  return images_;
}

Edges BoxShadows::overflow() const {
  Edges edges;
  for (const auto &shadow : shadows_) {
    const auto grow = reach(shadow);
    edges.left = std::max(edges.left, grow - shadow.offsetX);
    edges.right = std::max(edges.right, grow + shadow.offsetX);
    edges.top = std::max(edges.top, grow - shadow.offsetY);
    edges.bottom = std::max(edges.bottom, grow + shadow.offsetY);
  }
  return edges;
}

} // namespace rntizen
