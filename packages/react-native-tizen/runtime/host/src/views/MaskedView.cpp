#include "ComponentView.h"

#include "../mounting/ComponentViewRegistry.h"
#include "RenderSync.h"

namespace rntizen {

namespace {

// Evas renders an image clipper as a mask (efl-rendering.md section 3): the
// first child draws only into a proxy image whose alpha clips the others.
class MaskedView final : public ComponentView {
 public:
  MaskedView(Evas *evas, facebook::react::Tag tag) : ComponentView(evas, tag), mask_(evas_object_image_filled_add(evas)) {
    evas_object_image_source_clip_set(mask_, EINA_FALSE);
    evas_object_pass_events_set(mask_, EINA_TRUE);
    addMember(mask_, true);
    applyVisibility();
  }

  ~MaskedView() override {
    follow(nullptr);
    waitForRenderThread(mask_);
    evas_object_del(mask_);
  }

 private:
  Evas_Object *clipperFor(const ComponentView &child) const override {
    return source_ != nullptr && rootOf(child) != source_ ? mask_ : nullptr;
  }

  void didChangeChildren() override {
    follow(children().empty() ? nullptr : rootOf(*children().front()));
    refreshClips();
    applyVisibility();
  }

  static void onSourceGeometry(void *data, Evas *, Evas_Object *source, void *) {
    auto *self = static_cast<MaskedView *>(data);
    PixelRect rect;
    evas_object_geometry_get(source, &rect.x, &rect.y, &rect.width, &rect.height);
    setGeometry(self->mask_, rect);
  }

  static void onSourceDeleted(void *data, Evas *, Evas_Object *, void *) {
    auto *self = static_cast<MaskedView *>(data);
    waitForRenderThread(self->mask_);
    self->source_ = nullptr;
    evas_object_image_source_set(self->mask_, nullptr);
    self->refreshClips();
    self->applyVisibility();
  }

  void follow(Evas_Object *source) {
    if (source == source_) return;
    waitForRenderThread(mask_);
    if (source_ != nullptr) {
      evas_object_event_callback_del_full(source_, EVAS_CALLBACK_MOVE, onSourceGeometry, this);
      evas_object_event_callback_del_full(source_, EVAS_CALLBACK_RESIZE, onSourceGeometry, this);
      evas_object_event_callback_del_full(source_, EVAS_CALLBACK_DEL, onSourceDeleted, this);
      evas_object_image_source_visible_set(mask_, EINA_TRUE);
    }
    source_ = source;
    evas_object_image_source_set(mask_, source_);
    if (source_ == nullptr) return;
    evas_object_image_source_visible_set(mask_, EINA_FALSE);
    evas_object_event_callback_add(source_, EVAS_CALLBACK_MOVE, onSourceGeometry, this);
    evas_object_event_callback_add(source_, EVAS_CALLBACK_RESIZE, onSourceGeometry, this);
    evas_object_event_callback_add(source_, EVAS_CALLBACK_DEL, onSourceDeleted, this);
    onSourceGeometry(this, nullptr, source_, nullptr);
  }

  void applyVisibility() {
    if (source_ != nullptr && children().size() > 1) {
      evas_object_show(mask_);
    } else {
      evas_object_hide(mask_);
    }
  }

  Evas_Object *const mask_;
  Evas_Object *source_{nullptr};
};

} // namespace

RN_TIZEN_COMPONENT_VIEW("RNCMaskedView", MaskedView);

} // namespace rntizen
