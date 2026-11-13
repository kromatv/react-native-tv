#include "MaskedView.h"

#include "../mounting/ComponentViewRegistry.h"
#include "RenderSync.h"
#include "ViewSmartObject.h"

namespace rntizen {

// Evas renders an image clipper as a mask (efl-rendering.md section 3). The
// mask child draws only into the stage, a smart object the size of what the
// view shows on screen, and a proxy image of the stage clips the other
// children: the picture Evas keeps of the mask is never larger than the
// screen, however tall the view or the mask is.
MaskedView::MaskedView(Evas *evas, facebook::react::Tag tag)
    : ComponentView(evas, tag),
      stage_(addViewSmartObject(evas)),
      tick_(evas_object_rectangle_add(evas)),
      mask_(evas_object_image_filled_add(evas)) {
  evas_object_smart_member_add(tick_, stage_);
  evas_object_pass_events_set(tick_, EINA_TRUE);
  evas_object_show(tick_);
  evas_object_pass_events_set(stage_, EINA_TRUE);
  evas_object_pass_events_set(mask_, EINA_TRUE);
  evas_object_image_source_set(mask_, stage_);
  evas_object_image_source_visible_set(mask_, EINA_FALSE);
  evas_object_image_source_clip_set(mask_, EINA_FALSE);
  addMember(stage_, true);
  addMember(mask_, true);
  refreshMask();
}

MaskedView::~MaskedView() {
  waitForRenderThread(mask_);
  evas_object_del(mask_);
}

void MaskedView::layoutMembers() {
  PixelRect screen;
  evas_output_viewport_get(evas(), &screen.x, &screen.y, &screen.width, &screen.height);
  const auto shown = intersection(canvasRect(), screen);
  if (shown == shown_) return;
  shown_ = shown;
  setGeometry(stage_, shown_);
  setGeometry(mask_, shown_);
  // Evas draws a proxy's picture again when a member of its source changed,
  // never when the source itself moved or resized (efl-rendering.md section
  // 3): the old picture would be stretched into the new box. The tick, a
  // member kept outside the picture, moves with every change.
  ticked_ = !ticked_;
  evas_object_geometry_set(tick_, shown_.x - (ticked_ ? 2 : 1), shown_.y - 1, 1, 1);
  refreshMask();
}

Evas_Object *MaskedView::clipperFor(const ComponentView &child) const {
  return isMask(child) ? evas_object_smart_clipped_clipper_get(stage_) : mask_;
}

Evas_Object *MaskedView::hostFor(const ComponentView &child) const {
  return isMask(child) ? stage_ : nullptr;
}

void MaskedView::didChangeChildren() {
  rehostChildren();
  refreshMask();
}

bool MaskedView::isMask(const ComponentView &child) const {
  return !children().empty() && children().front() == &child;
}

void MaskedView::refreshMask() {
  if (children().size() > 1 && shown_.width > 0 && shown_.height > 0) {
    evas_object_show(mask_);
  } else {
    evas_object_hide(mask_);
  }
}

RN_TIZEN_COMPONENT_VIEW("RNCMaskedView", MaskedView);

} // namespace rntizen
