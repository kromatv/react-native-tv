#include "../../ExpoModuleRegistry.h"
#include "../../ModuleDefinition.h"
#include "../../views/ViewDefinition.h"
#include "BlurTint.h"

#include "views/BackdropBlur.h"
#include "views/BoxPainter.h"

#include <algorithm>

namespace rnwebos::expo {

namespace {

class ExpoBlurView final : public ExpoView {
 public:
  ExpoBlurView(Scene &scene, facebook::react::Tag tag) : ExpoView(scene, tag) {
    setAlwaysClips(true);
  }

  void setTint(const folly::dynamic &value) {
    tintName_ = value.isString() ? value.getString() : "default";
  }

  void setIntensity(const folly::dynamic &value) {
    intensity_ = value.isNumber() ? std::clamp(value.asDouble(), 0.0, 100.0) : 50.0;
  }

  void setBlurReductionFactor(const folly::dynamic &value) {
    reductionFactor_ = value.isNumber() && value.asDouble() > 0 ? value.asDouble() : 4.0;
  }

 private:
  void didUpdateProps() override {
    refresh();
  }

  void layoutContent() override {
    refresh();
  }

  void drawBackdrop(SkCanvas &canvas, float opacity) override {
    blur_.draw(canvas, shape_, opacity);
  }

  void drawContent(SkCanvas &canvas, float alpha) override {
    auto tint = SkColor4f::FromColor(tint_);
    tint.fA *= alpha;
    canvas.drawColor(tint);
  }

  int contentLayers() const override {
    return 1;
  }

  void refresh() {
    tint_ = blurTintColor(tintName_, intensity_);
    blur_.setSigma(blurSigmaPixels(intensity_, reductionFactor_, pointScale()));
    if (props()) {
      const auto &viewProps = static_cast<const facebook::react::ViewProps &>(*props());
      shape_ = roundedRect(pixelSize(), resolveBoxStyle(viewProps, layoutMetrics(), true).corners);
    }
    invalidate();
  }

  BackdropBlur blur_;
  SkRRect shape_;
  SkColor tint_{SK_ColorTRANSPARENT};
  std::string tintName_{"default"};
  double intensity_{50};
  double reductionFactor_{4};
};

class ExpoBlurTargetView final : public ExpoView {
 public:
  using ExpoView::ExpoView;
};

class ExpoBlurModule final : public Module {
 public:
  ModuleDefinition definition() override {
    return ModuleDefinition("ExpoBlur");
  }
};

} // namespace

RN_WEBOS_EXPO_MODULE(ExpoBlur, ExpoBlurModule);

RN_WEBOS_EXPO_VIEW(ViewDefinition<ExpoBlurView>("ExpoBlur", "ExpoBlurView")
                       .prop("tint", &ExpoBlurView::setTint)
                       .prop("intensity", &ExpoBlurView::setIntensity)
                       .prop("blurReductionFactor", &ExpoBlurView::setBlurReductionFactor));

RN_WEBOS_EXPO_VIEW(ViewDefinition<ExpoBlurTargetView>("ExpoBlur", "ExpoBlurTargetView"));

} // namespace rnwebos::expo
