#include "SurfacePresenter.h"

#include "../animated/AnimatedViewProps.h"
#include "../mounting/LogicalScreen.h"
#include "../threading/MainThread.h"
#include "ComponentDescriptorRegistry.h"
#include "InducingEventBeat.h"

#include <react/renderer/scheduler/SchedulerToolbox.h>

namespace rntizen {

namespace {

constexpr facebook::react::SurfaceId kSurfaceId = 1;
constexpr facebook::react::SurfaceId kRootTagIncrement = 10;

facebook::react::SchedulerToolbox makeToolbox(const SurfacePresenter::Dependencies &dependencies) {
  using namespace facebook::react;
  SchedulerToolbox toolbox{};
  toolbox.contextContainer = dependencies.contextContainer;
  toolbox.componentRegistryFactory = componentRegistryFactory();
  toolbox.bridgelessBindingsExecutor = dependencies.bridgelessBindingsExecutor;
  toolbox.runtimeExecutor = dependencies.runtimeExecutor;
  toolbox.eventBeatFactory = [runtimeScheduler = dependencies.runtimeScheduler](
                                 std::shared_ptr<EventBeat::OwnerBox> ownerBox) -> std::unique_ptr<EventBeat> {
    return std::make_unique<InducingEventBeat>(std::move(ownerBox), *runtimeScheduler);
  };
  return toolbox;
}

} // namespace

SurfacePresenter::SurfacePresenter(Evas *evas, Dependencies dependencies)
    : mountingManager_(evas),
      contextContainer_(dependencies.contextContainer),
      scheduler_(std::make_unique<facebook::react::Scheduler>(makeToolbox(dependencies), nullptr, this)),
      lastSurfaceId_(kSurfaceId),
      lifetime_(std::make_shared<SurfacePresenter *>(this)) {}

SurfacePresenter::~SurfacePresenter() {
  while (!overlays_.empty()) stopOverlaySurface(overlays_.begin()->first);
  stopSurface();
  scheduler_->setDelegate(nullptr);
}

MountingManager &SurfacePresenter::mountingManager() {
  return mountingManager_;
}

ComponentView *SurfacePresenter::rootView() const {
  return mountingManager_.findView(kSurfaceId);
}

std::unique_ptr<facebook::react::SurfaceHandler> SurfacePresenter::runSurface(
    const std::string &appKey,
    facebook::react::SurfaceId surfaceId,
    facebook::react::Size size) {
  using namespace facebook::react;
  mountingManager_.startSurface(surfaceId);
  auto surface = std::make_unique<SurfaceHandler>(appKey, surfaceId);
  surface->setContextContainer(contextContainer_);
  surface->setProps(folly::dynamic::object());
  surface->constraintLayout(
      LayoutConstraints{
          .minimumSize = kLogicalScreenSize,
          .maximumSize = kLogicalScreenSize,
          .layoutDirection = LayoutDirection::LeftToRight},
      LayoutContext{.pointScaleFactor = pointScaleFactor(size), .fontSizeMultiplier = 1});
  scheduler_->registerSurface(*surface);
  surface->start();
  return surface;
}

void SurfacePresenter::startSurface(const std::string &appKey, facebook::react::Size size) {
  surface_ = runSurface(appKey, kSurfaceId, size);
}

void SurfacePresenter::startOverlaySurface(const std::string &appKey, facebook::react::Size size) {
  if (overlays_.contains(appKey)) return;
  lastSurfaceId_ += kRootTagIncrement;
  overlays_[appKey] = runSurface(appKey, lastSurfaceId_, size);
}

void SurfacePresenter::stopOverlaySurface(const std::string &appKey) {
  auto found = overlays_.find(appKey);
  if (found == overlays_.end()) return;
  auto surface = std::move(found->second);
  overlays_.erase(found);
  // Stopping commits an empty tree and revokes its transaction before
  // returning: mounted inline, as iOS mounts a commit made on the main thread,
  // it deletes the surface's views while its root still holds them.
  mountsInline_ = true;
  surface->stop();
  mountsInline_ = false;
  scheduler_->unregisterSurface(*surface);
  mountingManager_.stopSurface(surface->getSurfaceId());
}

const facebook::react::SurfaceHandler *SurfacePresenter::surfaceWithId(facebook::react::SurfaceId surfaceId) const {
  if (surface_ && surface_->getSurfaceId() == surfaceId) return surface_.get();
  for (const auto &[appKey, overlay] : overlays_) {
    if (overlay->getSurfaceId() == surfaceId) return overlay.get();
  }
  return nullptr;
}

void SurfacePresenter::stopSurface() {
  if (!surface_) return;
  surface_->stop();
  scheduler_->unregisterSurface(*surface_);
  surface_.reset();
  mountingManager_.stopSurface(kSurfaceId);
}

void SurfacePresenter::mountOnMainThread(
    const std::shared_ptr<const facebook::react::MountingCoordinator> &mountingCoordinator) {
  runOnMainThread([lifetime = std::weak_ptr<SurfacePresenter *>(lifetime_), mountingCoordinator] {
    if (auto presenter = lifetime.lock()) (*presenter)->mountingManager_.performTransaction(*mountingCoordinator);
  });
}

void SurfacePresenter::schedulerDidFinishTransaction(
    const std::shared_ptr<const facebook::react::MountingCoordinator> &mountingCoordinator) {
  mountOnMainThread(mountingCoordinator);
}

void SurfacePresenter::schedulerShouldRenderTransactions(
    const std::shared_ptr<const facebook::react::MountingCoordinator> &mountingCoordinator) {
  if (isMainThread() && mountsInline_) {
    mountingManager_.performTransaction(*mountingCoordinator);
    return;
  }
  mountOnMainThread(mountingCoordinator);
}

void SurfacePresenter::schedulerShouldMergeReactRevision(facebook::react::SurfaceId) {}

void SurfacePresenter::schedulerDidRequestPreliminaryViewAllocation(const facebook::react::ShadowNode &) {}

void SurfacePresenter::schedulerDidDispatchCommand(
    const facebook::react::ShadowView &shadowView,
    const std::string &commandName,
    const folly::dynamic &args) {
  runOnMainThread([lifetime = std::weak_ptr<SurfacePresenter *>(lifetime_),
                   surfaceId = shadowView.surfaceId,
                   tag = shadowView.tag,
                   commandName,
                   args] {
    auto presenter = lifetime.lock();
    if (!presenter) return;
    auto &self = **presenter;
    // A command sent from an effect reaches here before the commit that made
    // its view is mounted; that commit is already in the coordinator.
    if (const auto *surface = self.surfaceWithId(surfaceId)) {
      self.mountingManager_.performTransaction(*surface->getMountingCoordinator());
    }
    self.mountingManager_.dispatchCommand(tag, commandName, args);
  });
}

void SurfacePresenter::schedulerDidSendAccessibilityEvent(const facebook::react::ShadowView &, const std::string &) {}

void SurfacePresenter::schedulerDidSetIsJSResponder(const facebook::react::ShadowView &, bool, bool) {}

void SurfacePresenter::schedulerShouldSynchronouslyUpdateViewOnUIThread(
    facebook::react::Tag tag,
    const folly::dynamic &props) {
  if (isMainThread()) {
    applyAnimatedProps(mountingManager_, *scheduler_, *contextContainer_, kSurfaceId, tag, props);
    return;
  }
  runOnMainThread([lifetime = std::weak_ptr<SurfacePresenter *>(lifetime_), tag, props] {
    if (auto presenter = lifetime.lock()) (*presenter)->schedulerShouldSynchronouslyUpdateViewOnUIThread(tag, props);
  });
}

void SurfacePresenter::schedulerDidUpdateShadowTree(const std::unordered_map<facebook::react::Tag, folly::dynamic> &) {}

void SurfacePresenter::schedulerDidCaptureViewSnapshot(facebook::react::Tag, facebook::react::SurfaceId) {}

void SurfacePresenter::schedulerDidSetViewSnapshot(facebook::react::Tag, facebook::react::Tag, facebook::react::SurfaceId) {}

void SurfacePresenter::schedulerDidClearPendingSnapshots() {}

} // namespace rntizen
