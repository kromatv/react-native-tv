#include "UnwrappedSizes.h"

namespace rntizen {

namespace {

constexpr std::size_t kMaxEntries = 1024;

} // namespace

std::optional<facebook::react::Size> UnwrappedSizes::find(const Key &key, int widthPixels) const {
  std::lock_guard lock(mutex_);
  const auto found = entries_.find(key);
  if (found == entries_.end() || widthPixels < found->second.widthPixels) return std::nullopt;
  return found->second.size;
}

void UnwrappedSizes::remember(const Key &key, int widthPixels, facebook::react::Size size) const {
  std::lock_guard lock(mutex_);
  if (entries_.size() >= kMaxEntries) entries_.clear();
  entries_.insert_or_assign(key, Entry{.widthPixels = widthPixels, .size = size});
}

} // namespace rntizen
