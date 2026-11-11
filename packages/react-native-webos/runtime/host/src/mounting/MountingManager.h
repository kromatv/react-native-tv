#pragma once

#include "../views/ComponentView.h"

#include <react/renderer/mounting/MountingCoordinator.h>
#include <react/renderer/mounting/ShadowViewMutation.h>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace rnwebos {

/// Turns Fabric's mutation lists into ComponentViews in one scene. Main thread
/// only.
class MountingManager final {
 public:
  explicit MountingManager(Scene &scene);

  MountingManager(const MountingManager &) = delete;
  MountingManager &operator=(const MountingManager &) = delete;

  /// Creates the surface's root view. Fabric mounts into it but never creates it:
  /// that is the platform's job, as on iOS and Android.
  /// `order` is the scene layer the surface draws at.
  void startSurface(facebook::react::SurfaceId surfaceId, int order);
  void stopSurface(facebook::react::SurfaceId surfaceId);

  /// Pulls whatever the coordinator has ready and applies it; a no-op when there is nothing new.
  void performTransaction(const facebook::react::MountingCoordinator &coordinator);

  ComponentView *findView(facebook::react::Tag tag) const;

  /// The component of the view `tag` was created for, unless it is a root.
  std::optional<facebook::react::ComponentHandle> componentHandle(facebook::react::Tag tag) const;

  /// Runs a view command (`scrollTo`...) on the mounted view `tag`; a command
  /// for a view that is not mounted is dropped.
  void dispatchCommand(facebook::react::Tag tag, const std::string &name, const folly::dynamic &args);

 private:
  void apply(const facebook::react::ShadowViewMutation &mutation);
  void create(const facebook::react::ShadowView &shadowView);
  void update(const facebook::react::ShadowView &oldView, const facebook::react::ShadowView &newView);

  Scene &scene_;
  std::unordered_map<facebook::react::Tag, std::unique_ptr<ComponentView>> views_;
  std::unordered_map<facebook::react::Tag, facebook::react::ComponentHandle> handles_;
};

} // namespace rnwebos
