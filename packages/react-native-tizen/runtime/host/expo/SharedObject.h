#pragma once

#include <atomic>
#include <memory>

namespace rntizen::expo {

/// The id JavaScript sees on a shared object: 1 and up, 0 while it is not paired.
using SharedObjectId = long;

/// A native object JavaScript holds as an instance of an Expo class: Expo's
/// SharedObject. The SharedObjectRegistry keeps it alive until JavaScript
/// calls `release()` or collects the JavaScript object. Native code must not
/// keep JavaScript values in it.
class SharedObject : public std::enable_shared_from_this<SharedObject> {
 public:
  SharedObject() = default;
  virtual ~SharedObject() = default;

  SharedObject(const SharedObject &) = delete;
  SharedObject &operator=(const SharedObject &) = delete;

  SharedObjectId sharedObjectId() const {
    return id_.load();
  }

  /// Runs once the registry dropped the object, on the thread that released
  /// it, which may be the JavaScript thread inside a garbage collection: never
  /// touch the runtime here.
  virtual void sharedObjectDidRelease() {}

 private:
  friend class SharedObjectRegistry;
  std::atomic<SharedObjectId> id_{0};
};

} // namespace rntizen::expo
