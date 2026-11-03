#include "SharedObjectRegistry.h"

#include <utility>

namespace rntizen::expo {

SharedObjectRegistry &SharedObjectRegistry::shared() {
  static auto *registry = new SharedObjectRegistry();
  return *registry;
}

SharedObjectId SharedObjectRegistry::add(std::shared_ptr<SharedObject> object) {
  std::lock_guard lock(mutex_);
  const auto id = nextId_++;
  object->id_.store(id);
  objects_.emplace(id, std::move(object));
  return id;
}

std::shared_ptr<SharedObject> SharedObjectRegistry::find(SharedObjectId id) const {
  std::lock_guard lock(mutex_);
  auto found = objects_.find(id);
  return found == objects_.end() ? nullptr : found->second;
}

void SharedObjectRegistry::release(SharedObjectId id) {
  std::shared_ptr<SharedObject> object;
  {
    std::lock_guard lock(mutex_);
    auto found = objects_.find(id);
    if (found == objects_.end()) return;
    object = std::move(found->second);
    objects_.erase(found);
    released_.push_back(id);
  }
  object->id_.store(0);
  object->sharedObjectDidRelease();
}

std::vector<SharedObjectId> SharedObjectRegistry::takeReleased() {
  std::lock_guard lock(mutex_);
  return std::exchange(released_, {});
}

} // namespace rntizen::expo
