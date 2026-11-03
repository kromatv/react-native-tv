#include "MountingManager.h"

#include "../diagnostics/TransactionStats.h"
#include "../logging/Log.h"
#include "../threading/MainThread.h"
#include "ComponentViewRegistry.h"

#include <chrono>
#include <map>
#include <string>

namespace rntizen {

namespace {

const char *componentNameOf(const facebook::react::ShadowViewMutation &mutation) {
  using Type = facebook::react::ShadowViewMutation::Type;
  const bool removes = mutation.type == Type::Remove || mutation.type == Type::Delete;
  const auto *name = removes ? mutation.oldChildShadowView.componentName : mutation.newChildShadowView.componentName;
  return name != nullptr ? name : "?";
}

} // namespace

MountingManager::MountingManager(Evas *evas) : evas_(evas) {}

ComponentView *MountingManager::findView(facebook::react::Tag tag) const {
  auto found = views_.find(tag);
  return found == views_.end() ? nullptr : found->second.get();
}

std::optional<facebook::react::ComponentHandle> MountingManager::componentHandle(facebook::react::Tag tag) const {
  auto found = handles_.find(tag);
  if (found == handles_.end()) return std::nullopt;
  return found->second;
}

void MountingManager::dispatchCommand(facebook::react::Tag tag, const std::string &name, const folly::dynamic &args) {
  auto *view = findView(tag);
  if (view == nullptr) {
    log(LogLevel::Debug, "Dropping " + name + " for view " + std::to_string(tag) + ", which is not mounted");
    return;
  }
  view->handleCommand(name, args);
}

void MountingManager::startSurface(facebook::react::SurfaceId surfaceId) {
  auto root = std::make_unique<ComponentView>(evas_, surfaceId);
  root->mountAsRoot();
  views_[surfaceId] = std::move(root);
}

void MountingManager::stopSurface(facebook::react::SurfaceId surfaceId) {
  views_.erase(surfaceId);
}

void MountingManager::performTransaction(const facebook::react::MountingCoordinator &coordinator) {
  const bool measured = transactionStatsEnabled();
  const auto started = measured ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
  auto transaction = coordinator.pullTransaction();
  if (!transaction.has_value()) return;
  if (!measured) {
    for (const auto &mutation : transaction->getMutations()) {
      apply(mutation);
      runPriorityTasks();
    }
    return;
  }
  std::map<std::string, ComponentMountCost> components;
  for (const auto &mutation : transaction->getMutations()) {
    const auto before = std::chrono::steady_clock::now();
    apply(mutation);
    auto &cost = components[componentNameOf(mutation)];
    cost.mutations += 1;
    cost.ms += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - before).count();
    runPriorityTasks();
  }
  recordTransaction(
      transaction->getTelemetry(),
      static_cast<int>(transaction->getMutations().size()),
      started,
      std::chrono::steady_clock::now(),
      components);
}

void MountingManager::create(const facebook::react::ShadowView &shadowView) {
  auto view = createComponentView(evas_, shadowView);
  view->updateProps(shadowView.props);
  view->updateEventEmitter(shadowView.eventEmitter);
  view->updateState(shadowView.state);
  view->updateLayout(shadowView.layoutMetrics);
  views_[shadowView.tag] = std::move(view);
  handles_[shadowView.tag] = shadowView.componentHandle;
}

void MountingManager::update(const facebook::react::ShadowView &oldView, const facebook::react::ShadowView &newView) {
  auto *view = findView(newView.tag);
  if (view == nullptr) return;
  if (oldView.props != newView.props) view->updateProps(newView.props);
  if (oldView.eventEmitter != newView.eventEmitter) view->updateEventEmitter(newView.eventEmitter);
  if (oldView.state != newView.state) view->updateState(newView.state);
  if (oldView.layoutMetrics != newView.layoutMetrics) view->updateLayout(newView.layoutMetrics);
}

void MountingManager::apply(const facebook::react::ShadowViewMutation &mutation) {
  using Type = facebook::react::ShadowViewMutation::Type;
  switch (mutation.type) {
    case Type::Create:
      create(mutation.newChildShadowView);
      return;
    case Type::Delete:
      views_.erase(mutation.oldChildShadowView.tag);
      handles_.erase(mutation.oldChildShadowView.tag);
      return;
    case Type::Insert: {
      auto *parent = findView(mutation.parentTag);
      auto *child = findView(mutation.newChildShadowView.tag);
      if (parent == nullptr || child == nullptr) {
        log(LogLevel::Warning,
            "Cannot insert view " + std::to_string(mutation.newChildShadowView.tag) + " into " +
                std::to_string(mutation.parentTag) + ": " + (parent == nullptr ? "no parent" : "no child"));
        return;
      }
      if (child->layoutMetrics() != mutation.newChildShadowView.layoutMetrics) {
        child->updateLayout(mutation.newChildShadowView.layoutMetrics);
      }
      parent->mountChild(*child, static_cast<std::size_t>(mutation.index));
      return;
    }
    case Type::Remove: {
      auto *parent = findView(mutation.parentTag);
      auto *child = findView(mutation.oldChildShadowView.tag);
      if (parent != nullptr && child != nullptr) parent->unmountChild(*child);
      return;
    }
    case Type::Update:
      update(mutation.oldChildShadowView, mutation.newChildShadowView);
      return;
  }
}

} // namespace rntizen
