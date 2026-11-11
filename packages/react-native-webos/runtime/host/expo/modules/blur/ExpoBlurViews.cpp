#include "../../ExpoModuleRegistry.h"
#include "../../ModuleDefinition.h"
#include "../../views/ViewDefinition.h"
#include "BlurTint.h"

#include "views/BackdropBlur.h"

#include <algorithm>

namespace rntizen::expo {

namespace {

class ExpoBlurView final : public ExpoView {
 public:
  ExpoBlurView(Evas *evas, facebook::react::Tag tag)
      : ExpoView(evas, tag), blur_(evas), tint_(evas_object_rectangle_add(evas)) {
    setAlwaysClips(true);
    if (blur_.object() != nullptr) addMember(blur_.object(), true);
    evas_object_pass_events_set(tint_, EINA_TRUE);
    addMember(tint_, true);
    paint();
  }

  ~ExpoBlurView() override {
    evas_object_del(tint_);
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

 protected:
  void didUpdateProps() override {
    paint();
  }

  void layoutMembers() override {
    ExpoView::layoutMembers();
    blur_.setRadius(blurRadiusPixels(intensity_, reductionFactor_, pointScale()));
    blur_.layout(canvasRect());
    setGeometry(tint_, canvasRect());
  }

 private:
  void paint() {
    const auto color = blurTintColor(tintName_, intensity_);
    evas_object_color_set(tint_, color.red, color.green, color.blue, color.alpha);
    blur_.setRadius(blurRadiusPixels(intensity_, reductionFactor_, pointScale()));
  }

  BackdropBlur blur_;
  Evas_Object *const tint_;
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

RN_TIZEN_EXPO_MODULE(ExpoBlur, ExpoBlurModule);

RN_TIZEN_EXPO_VIEW(ViewDefinition<ExpoBlurView>("ExpoBlur", "ExpoBlurView")
                       .prop("tint", &ExpoBlurView::setTint)
                       .prop("intensity", &ExpoBlurView::setIntensity)
                       .prop("blurReductionFactor", &ExpoBlurView::setBlurReductionFactor));

RN_TIZEN_EXPO_VIEW(ViewDefinition<ExpoBlurTargetView>("ExpoBlur", "ExpoBlurTargetView"));

} // namespace rntizen::expo
