#pragma once

#include "../mounting/MountingManager.h"

#include <ReactCommon/RuntimeExecutor.h>
#include <react/renderer/runtimescheduler/RuntimeScheduler.h>
#include <react/renderer/scheduler/Scheduler.h>
#include <react/renderer/scheduler/SurfaceHandler.h>
#include <react/utils/ContextContainer.h>

#include <Evas.h>

#include <map>
#include <memory>
#include <string>

namespace rntizen {

/// Runs one React Native surface with Fabric and mounts it on an Evas canvas.
/// Fabric commits on the JavaScript thread; each finished transaction is
/// mounted on the main loop. Create, use and destroy it on the main thread.
class SurfacePresenter final : public facebook::react::SchedulerDelegate {
 public:
  struct Dependencies {
    std::shared_ptr<const facebook::react::ContextContainer> contextContainer;
    std::shared_ptr<facebook::react::RuntimeScheduler> runtimeScheduler;
    facebook::react::RuntimeExecutor runtimeExecutor;
    facebook::react::RuntimeExecutor bridgelessBindingsExecutor;
  };

  SurfacePresenter(Evas *evas, Dependencies dependencies);
  ~SurfacePresenter() override;

  SurfacePresenter(const SurfacePresenter &) = delete;
  SurfacePresenter &operator=(const SurfacePresenter &) = delete;

  /// Runs `appKey` (what `AppRegistry.registerComponent`, or Expo's
  /// `registerRootComponent` as `main`, registered) across `size`.
  void startSurface(const std::string &appKey, facebook::react::Size size);
  void stopSurface();

  /// Runs `appKey` in a surface of its own over the app's, as LogBox's
  /// inspector runs; a no-op while one runs it already.
  void startOverlaySurface(const std::string &appKey, facebook::react::Size size);
  /// Stops the overlay surface running `appKey`, deleting its views first.
  void stopOverlaySurface(const std::string &appKey);

  MountingManager &mountingManager();

  /// The surface's root view, or null before the surface starts.
  ComponentView *rootView() const;

  void schedulerDidFinishTransaction(
      const std::shared_ptr<const facebook::react::MountingCoordinator> &mountingCoordinator) override;
  void schedulerShouldRenderTransactions(
      const std::shared_ptr<const facebook::react::MountingCoordinator> &mountingCoordinator) override;
  void schedulerShouldMergeReactRevision(facebook::react::SurfaceId surfaceId) override;
  void schedulerDidRequestPreliminaryViewAllocation(const facebook::react::ShadowNode &shadowNode) override;
  void schedulerDidDispatchCommand(
      const facebook::react::ShadowView &shadowView,
      const std::string &commandName,
      const folly::dynamic &args) override;
  void schedulerDidSendAccessibilityEvent(
      const facebook::react::ShadowView &shadowView,
      const std::string &eventType) override;
  void schedulerDidSetIsJSResponder(
      const facebook::react::ShadowView &shadowView,
      bool isJSResponder,
      bool blockNativeResponder) override;
  void schedulerShouldSynchronouslyUpdateViewOnUIThread(facebook::react::Tag tag, const folly::dynamic &props) override;
  void schedulerDidUpdateShadowTree(const std::unordered_map<facebook::react::Tag, folly::dynamic> &tagToProps) override;
  void schedulerDidCaptureViewSnapshot(facebook::react::Tag tag, facebook::react::SurfaceId surfaceId) override;
  void schedulerDidSetViewSnapshot(
      facebook::react::Tag sourceTag,
      facebook::react::Tag targetTag,
      facebook::react::SurfaceId surfaceId) override;
  void schedulerDidClearPendingSnapshots() override;

 private:
  void mountOnMainThread(const std::shared_ptr<const facebook::react::MountingCoordinator> &mountingCoordinator);
  std::unique_ptr<facebook::react::SurfaceHandler>
  runSurface(const std::string &appKey, facebook::react::SurfaceId surfaceId, facebook::react::Size size);
  const facebook::react::SurfaceHandler *surfaceWithId(facebook::react::SurfaceId surfaceId) const;

  MountingManager mountingManager_;
  std::shared_ptr<const facebook::react::ContextContainer> contextContainer_;
  std::unique_ptr<facebook::react::Scheduler> scheduler_;
  std::unique_ptr<facebook::react::SurfaceHandler> surface_;
  std::map<std::string, std::unique_ptr<facebook::react::SurfaceHandler>, std::less<>> overlays_;
  facebook::react::SurfaceId lastSurfaceId_;
  bool mountsInline_{false};
  std::shared_ptr<SurfacePresenter *> lifetime_;
};

} // namespace rntizen
