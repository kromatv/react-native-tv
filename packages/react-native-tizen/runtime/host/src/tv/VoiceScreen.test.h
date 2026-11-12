#pragma once

#include "../mounting/LogicalScreen.h"
#include "../views/ComponentView.h"

#include <doctest/doctest.h>
#include <hermes/hermes.h>
#include <react/renderer/components/view/TouchEventEmitter.h>
#include <react/renderer/components/view/ViewProps.h>
#include <react/renderer/core/EventDispatcher.h>
#include <react/renderer/core/EventQueueProcessor.h>
#include <react/renderer/runtimescheduler/RuntimeScheduler.h>

#include <Ecore_Evas.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace rntizen::testing {

/// A view a test mounts: its label, its frame in points, and what may hide it.
struct ViewLook {
  std::string label;
  facebook::react::Rect frame{.origin = {.x = 0, .y = 0}, .size = {.width = 200, .height = 100}};
  facebook::react::Float opacity{1};
  facebook::react::DisplayType display{facebook::react::DisplayType::Flex};
  bool touchable{true};
};

/// A touch event a view sent: its type, the view it targets and its first changed touch on the page.
struct SentTouch {
  std::string type;
  facebook::react::Tag target;
  double pageX;
  double pageY;

  bool operator==(const SentTouch &) const = default;
};

/// A 1920x1080 root view laid out at `scale` pixels per point on a buffer
/// canvas, the views a test mounts under it, and the touch events they send.
class VoiceScreen final {
 public:
  explicit VoiceScreen(facebook::react::Float scale = 1)
      : scale_(scale),
        canvas_(newCanvas(), ecore_evas_free),
        scheduler_([](std::function<void(facebook::jsi::Runtime &)> &&) {}),
        dispatcher_(std::make_shared<const facebook::react::EventDispatcher>(
            facebook::react::EventQueueProcessor(nullptr, nullptr, nullptr, {}),
            std::make_unique<facebook::react::EventBeat>(
                std::make_shared<facebook::react::EventBeat::OwnerBox>(), scheduler_),
            nullptr,
            std::weak_ptr<facebook::react::EventLogger>{})),
        listener_(std::make_shared<const facebook::react::EventListener>(
            [this](const facebook::react::RawEvent &event) { return record(event); })),
        root_(std::make_unique<ComponentView>(ecore_evas_get(canvas_.get()), 1)) {
    dispatcher_->addListener(listener_);
    style(*root_, {.label = "", .frame = {.origin = {.x = 0, .y = 0}, .size = kLogicalScreenSize}});
    root_->mountAsRoot();
  }

  ComponentView &root() const {
    return *root_;
  }

  ComponentView &add(const ViewLook &look) {
    return add(*root_, look);
  }

  ComponentView &add(ComponentView &parent, const ViewLook &look) {
    auto &view = create();
    style(view, look);
    parent.mountChild(view, parent.children().size());
    return view;
  }

  ComponentView &addWithoutProps() {
    auto &view = create();
    root_->mountChild(view, root_->children().size());
    return view;
  }

  const std::vector<SentTouch> &touches() const {
    return touches_;
  }

 private:
  static Ecore_Evas *newCanvas() {
    static const bool initialized = ecore_evas_init() > 0;
    REQUIRE(initialized);
    return ecore_evas_buffer_new(static_cast<int>(kLogicalScreenSize.width), static_cast<int>(kLogicalScreenSize.height));
  }

  ComponentView &create() {
    views_.push_back(std::make_unique<ComponentView>(ecore_evas_get(canvas_.get()), nextTag_++));
    return *views_.back();
  }

  void style(ComponentView &view, const ViewLook &look) const {
    auto props = std::make_shared<facebook::react::ViewProps>();
    props->accessibilityLabel = look.label;
    props->opacity = look.opacity;
    view.updateProps(props);
    facebook::react::LayoutMetrics metrics{};
    metrics.frame = look.frame;
    metrics.pointScaleFactor = scale_;
    metrics.displayType = look.display;
    view.updateLayout(metrics);
    if (look.touchable) view.updateEventEmitter(std::make_shared<facebook::react::TouchEventEmitter>(nullptr, dispatcher_));
  }

  bool record(const facebook::react::RawEvent &event) {
    auto &runtime = *runtime_;
    const auto payload = event.eventPayload->asJSIValue(runtime).asObject(runtime);
    touches_.push_back({
        .type = event.type,
        .target = static_cast<facebook::react::Tag>(payload.getProperty(runtime, "target").asNumber()),
        .pageX = payload.getProperty(runtime, "pageX").asNumber(),
        .pageY = payload.getProperty(runtime, "pageY").asNumber(),
    });
    return true;
  }

  const facebook::react::Float scale_;
  const std::unique_ptr<Ecore_Evas, decltype(&ecore_evas_free)> canvas_;
  const std::unique_ptr<facebook::hermes::HermesRuntime> runtime_{facebook::hermes::makeHermesRuntime()};
  facebook::react::RuntimeScheduler scheduler_;
  const std::shared_ptr<const facebook::react::EventDispatcher> dispatcher_;
  const std::shared_ptr<const facebook::react::EventListener> listener_;
  const std::unique_ptr<ComponentView> root_;
  std::vector<std::unique_ptr<ComponentView>> views_;
  facebook::react::Tag nextTag_{2};
  std::vector<SentTouch> touches_;
};

} // namespace rntizen::testing
