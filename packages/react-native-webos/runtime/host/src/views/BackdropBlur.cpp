#include "BackdropBlur.h"

#include "EvasFilter.h"
#include "RenderSync.h"

namespace rntizen {

namespace {

constexpr const char *kBlurProgram = "padding_set { 0 } blur { radius }";

Evas_Object *addSnapshot(Evas *evas) {
  if (!evasFiltersAvailable()) return nullptr;
  auto *snapshot = evas_object_image_filled_add(evas);
  evas_object_image_snapshot_set(snapshot, EINA_TRUE);
  evas_object_pass_events_set(snapshot, EINA_TRUE);
  setEvasFilterNumber(snapshot, "radius", 0);
  setEvasFilterProgram(snapshot, kBlurProgram);
  return snapshot;
}

} // namespace

BackdropBlur::BackdropBlur(Evas *evas) : snapshot_(addSnapshot(evas)) {}

BackdropBlur::~BackdropBlur() {
  if (snapshot_ == nullptr) return;
  waitForRenderThread(snapshot_);
  evas_object_del(snapshot_);
}

Evas_Object *BackdropBlur::object() const {
  return snapshot_;
}

void BackdropBlur::setRadius(int pixels) {
  if (snapshot_ == nullptr || pixels == radius_) return;
  radius_ = pixels;
  waitForRenderThread(snapshot_);
  setEvasFilterNumber(snapshot_, "radius", radius_);
  layout(box_);
}

void BackdropBlur::layout(const PixelRect &box) {
  box_ = box;
  if (snapshot_ == nullptr) return;
  setGeometry(snapshot_, inflated(box_, radius_, radius_, radius_, radius_));
  applyVisibility();
}

void BackdropBlur::applyVisibility() {
  if (radius_ > 0 && box_.width > 0 && box_.height > 0) {
    evas_object_show(snapshot_);
  } else {
    evas_object_hide(snapshot_);
  }
}

} // namespace rntizen
