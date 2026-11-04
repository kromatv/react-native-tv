#pragma once

#include "../mounting/LogicalScreen.h"
#include "../mounting/MountingManager.h"

#include <doctest/doctest.h>
#include <hermes/hermes.h>
#include <react/renderer/componentregistry/ComponentDescriptorProviderRegistry.h>
#include <react/renderer/components/view/ViewComponentDescriptor.h>
#include <react/renderer/runtimescheduler/RuntimeScheduler.h>
#include <react/renderer/scheduler/Scheduler.h>
#include <react/renderer/scheduler/SchedulerDelegate.h>
#include <react/renderer/scheduler/SchedulerToolbox.h>
#include <react/renderer/scheduler/SurfaceHandler.h>
#include <react/utils/ContextContainer.h>

#include <Ecore_Evas.h>

#include <functional>
#include <memory>
#include <string>

namespace rntizen::testing {

/// Fabric on a Hermes runtime with one empty surface, mounted by a
/// MountingManager on a buffer canvas as soon as JavaScript commits through
/// `nativeFabricUIManager`.
class FabricSurface final : public facebook::react::SchedulerDelegate {
 public:
  static constexpr facebook::react::SurfaceId kSurfaceId = 1;

  FabricSurface()
      : providers_(viewProviders()),
        canvas_(newCanvas(), ecore_evas_free),
        mountingManager_(ecore_evas_get(canvas_.get())),
        runtimeScheduler_(std::make_shared<facebook::react::RuntimeScheduler>(executor())),
        contextContainer_(contextWith(runtimeScheduler_)),
        scheduler_(toolbox(), nullptr, this),
        surface_("", kSurfaceId) {
    evaluate("globalThis.RN$stopSurface = function () {};");
    surface_.setProps(folly::dynamic::object());
    surface_.constraintLayout(
        {.minimumSize = kLogicalScreenSize,
         .maximumSize = kLogicalScreenSize,
         .layoutDirection = facebook::react::LayoutDirection::LeftToRight},
        {.pointScaleFactor = 1, .fontSizeMultiplier = 1});
    mountingManager_.startSurface(kSurfaceId);
    scheduler_.registerSurface(surface_);
    surface_.start();
  }

  ~FabricSurface() override {
    surface_.stop();
    scheduler_.unregisterSurface(surface_);
    mountingManager_.stopSurface(kSurfaceId);
  }

  FabricSurface(const FabricSurface &) = delete;
  FabricSurface &operator=(const FabricSurface &) = delete;

  /// Commits one plain `View` with `props`, as a JavaScript object literal, under the surface's root.
  void mountView(facebook::react::Tag tag, const std::string &props) {
    evaluate(
        "const view = nativeFabricUIManager.createNode(" + std::to_string(tag) + ", 'RCTView', " +
        std::to_string(kSurfaceId) + ", " + props +
        ", {});"
        "const children = nativeFabricUIManager.createChildSet(" +
        std::to_string(kSurfaceId) +
        ");"
        "nativeFabricUIManager.appendChildToSet(children, view);"
        "nativeFabricUIManager.completeRoot(" +
        std::to_string(kSurfaceId) + ", children);");
  }

  MountingManager &mountingManager() {
    return mountingManager_;
  }

  const facebook::react::Scheduler &scheduler() const {
    return scheduler_;
  }

  const facebook::react::ContextContainer &contextContainer() const {
    return *contextContainer_;
  }

  void schedulerDidFinishTransaction(
      const std::shared_ptr<const facebook::react::MountingCoordinator> &coordinator) override {
    mountingManager_.performTransaction(*coordinator);
  }
  void schedulerShouldRenderTransactions(
      const std::shared_ptr<const facebook::react::MountingCoordinator> &coordinator) override {
    mountingManager_.performTransaction(*coordinator);
  }
  void schedulerShouldMergeReactRevision(facebook::react::SurfaceId) override {}
  void schedulerDidRequestPreliminaryViewAllocation(const facebook::react::ShadowNode &) override {}
  void schedulerDidDispatchCommand(
      const facebook::react::ShadowView &, const std::string &, const folly::dynamic &) override {}
  void schedulerDidSendAccessibilityEvent(const facebook::react::ShadowView &, const std::string &) override {}
  void schedulerDidSetIsJSResponder(const facebook::react::ShadowView &, bool, bool) override {}
  void schedulerShouldSynchronouslyUpdateViewOnUIThread(facebook::react::Tag, const folly::dynamic &) override {}
  void schedulerDidUpdateShadowTree(const std::unordered_map<facebook::react::Tag, folly::dynamic> &) override {}
  void schedulerDidCaptureViewSnapshot(facebook::react::Tag, facebook::react::SurfaceId) override {}
  void schedulerDidSetViewSnapshot(facebook::react::Tag, facebook::react::Tag, facebook::react::SurfaceId) override {}
  void schedulerDidClearPendingSnapshots() override {}

 private:
  static std::unique_ptr<facebook::react::ComponentDescriptorProviderRegistry> viewProviders() {
    auto providers = std::make_unique<facebook::react::ComponentDescriptorProviderRegistry>();
    providers->add(facebook::react::concreteComponentDescriptorProvider<facebook::react::ViewComponentDescriptor>());
    return providers;
  }

  static Ecore_Evas *newCanvas() {
    static const bool initialized = ecore_evas_init() > 0;
    REQUIRE(initialized);
    return ecore_evas_buffer_new(static_cast<int>(kLogicalScreenSize.width), static_cast<int>(kLogicalScreenSize.height));
  }

  static std::shared_ptr<const facebook::react::ContextContainer> contextWith(
      const std::shared_ptr<facebook::react::RuntimeScheduler> &runtimeScheduler) {
    auto context = std::make_shared<const facebook::react::ContextContainer>();
    context->insert(facebook::react::RuntimeSchedulerKey, std::weak_ptr<facebook::react::RuntimeScheduler>(runtimeScheduler));
    return context;
  }

  facebook::react::RuntimeExecutor executor() {
    return [this](std::function<void(facebook::jsi::Runtime &)> &&callback) { callback(*runtime_); };
  }

  facebook::react::SchedulerToolbox toolbox() {
    facebook::react::SchedulerToolbox toolbox{};
    toolbox.contextContainer = contextContainer_;
    toolbox.componentRegistryFactory = [this](
                                           const facebook::react::EventDispatcher::Weak &eventDispatcher,
                                           const std::shared_ptr<const facebook::react::ContextContainer> &context) {
      return providers_->createComponentDescriptorRegistry(
          {.eventDispatcher = eventDispatcher, .contextContainer = context});
    };
    toolbox.runtimeExecutor = executor();
    toolbox.eventBeatFactory = [this](std::shared_ptr<facebook::react::EventBeat::OwnerBox> ownerBox) {
      return std::make_unique<facebook::react::EventBeat>(std::move(ownerBox), *runtimeScheduler_);
    };
    return toolbox;
  }

  void evaluate(const std::string &source) {
    runtime_->evaluateJavaScript(std::make_shared<facebook::jsi::StringBuffer>("(function () {" + source + "})()"), "test.js");
  }

  const std::unique_ptr<const facebook::react::ComponentDescriptorProviderRegistry> providers_;
  const std::unique_ptr<facebook::hermes::HermesRuntime> runtime_{facebook::hermes::makeHermesRuntime()};
  const std::unique_ptr<Ecore_Evas, decltype(&ecore_evas_free)> canvas_;
  MountingManager mountingManager_;
  const std::shared_ptr<facebook::react::RuntimeScheduler> runtimeScheduler_;
  const std::shared_ptr<const facebook::react::ContextContainer> contextContainer_;
  facebook::react::Scheduler scheduler_;
  facebook::react::SurfaceHandler surface_;
};

} // namespace rntizen::testing
