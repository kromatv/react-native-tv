#include "SvgView.h"

#include "../mounting/ComponentViewRegistry.h"
#include "SvgGradientView.h"

#include <react/renderer/components/rnsvg/Props.h>

namespace rntizen::svg {

namespace {

template <typename Props>
class SvgRootView final : public SvgView {
 public:
  using SvgView::SvgView;

 protected:
  void readProps(const facebook::react::Props &props) override {
    readSvgProps(static_cast<const Props &>(props));
  }
};

} // namespace

SvgView::SvgView(Evas *evas, facebook::react::Tag tag)
    : ComponentView(evas, tag),
      canvas_(evas),
      handle_(std::make_shared<SvgView *const>(this)) {
  addMember(canvas_.object(), true);
  setAlwaysClips(true);
}

SvgView::~SvgView() {
  if (job_ != nullptr) ecore_job_del(job_);
}

void SvgView::updateProps(const facebook::react::Props::Shared &props) {
  ComponentView::updateProps(props);
  readProps(*props);
  scheduleRedraw();
}

std::shared_ptr<SvgView *const> SvgView::handle() const {
  return handle_;
}

void SvgView::layoutMembers() {
  ComponentView::layoutMembers();
  const auto &rect = canvasRect();
  setGeometry(canvas_.object(), rect);
  if (rect.width != painted_.width || rect.height != painted_.height) scheduleRedraw();
}

void SvgView::onRedrawJob(void *data) {
  auto *view = static_cast<SvgView *>(data);
  view->job_ = nullptr;
  view->redraw();
}

void SvgView::scheduleRedraw() noexcept {
  if (job_ == nullptr) job_ = ecore_job_add(&SvgView::onRedrawJob, this);
}

void SvgView::redraw() {
  const auto &size = layoutMetrics().frame.size;
  const double scale = pointScale();
  GradientDefinitions gradients;
  collectGradients(children(), gradients);
  const DrawContext context{
      .transform = Affine::scaling(scale, scale) * viewBoxTransform(viewBox_, size.width, size.height),
      .currentColor = color_,
      .viewportWidth = viewBox_.present() ? viewBox_.width : size.width,
      .viewportHeight = viewBox_.present() ? viewBox_.height : size.height,
      .gradients = &gradients,
  };
  Drawing drawing;
  drawChildren(children(), drawing, context);
  painted_ = canvasRect();
  canvas_.paint(drawing, painted_.width, painted_.height);
}

RN_TIZEN_COMPONENT_VIEW("RNSVGSvgView", SvgRootView<facebook::react::RNSVGSvgViewProps>);
RN_TIZEN_COMPONENT_VIEW("RNSVGSvgViewAndroid", SvgRootView<facebook::react::RNSVGSvgViewAndroidProps>);

} // namespace rntizen::svg
