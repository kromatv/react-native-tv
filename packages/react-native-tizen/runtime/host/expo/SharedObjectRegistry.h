#pragma once

#include "SharedObject.h"

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace rntizen::expo {

/// Every native object paired with a JavaScript one, by id, for the whole
/// process: ids start at 1 and are never reused, so a stale id finds nothing.
/// Thread-safe, as on iOS and Android.
class SharedObjectRegistry final {
 public:
  static SharedObjectRegistry &shared();

  /// Takes `object`, gives it the next id and returns that id.
  SharedObjectId add(std::shared_ptr<SharedObject> object);

  /// The live object with `id`, or null once it was released.
  std::shared_ptr<SharedObject> find(SharedObjectId id) const;

  template <typename T>
  std::shared_ptr<T> find(SharedObjectId id) const {
    return std::dynamic_pointer_cast<T>(find(id));
  }

  /// Drops the object with `id`, resets its id to 0 and calls
  /// `sharedObjectDidRelease`. Does nothing for an id already released.
  void release(SharedObjectId id);

  /// The ids released since the last call, so the JavaScript side can forget them.
  std::vector<SharedObjectId> takeReleased();

 private:
  mutable std::mutex mutex_;
  SharedObjectId nextId_{1};
  std::unordered_map<SharedObjectId, std::shared_ptr<SharedObject>> objects_;
  std::vector<SharedObjectId> released_;
};

} // namespace rntizen::expo
