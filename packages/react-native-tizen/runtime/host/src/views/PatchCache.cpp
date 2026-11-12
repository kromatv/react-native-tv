#include "PatchCache.h"

#include <cstdint>
#include <utility>

namespace rntizen {

namespace {

constexpr std::size_t kSharedBudgetBytes = 24 << 20;

std::size_t bytesOf(const NinePatch &patch) {
  return patch.pixels.size() * sizeof(std::uint32_t);
}

} // namespace

PatchKey patchKey(PatchKind kind, const Corners &corners) {
  const auto &[tl, tr, br, bl] = corners;
  return {
      static_cast<float>(kind),
      static_cast<float>(tl.x),
      static_cast<float>(tl.y),
      static_cast<float>(tr.x),
      static_cast<float>(tr.y),
      static_cast<float>(br.x),
      static_cast<float>(br.y),
      static_cast<float>(bl.x),
      static_cast<float>(bl.y),
  };
}

void appendEdges(PatchKey &key, const Edges &edges) {
  key.insert(
      key.end(),
      {static_cast<float>(edges.left),
       static_cast<float>(edges.top),
       static_cast<float>(edges.right),
       static_cast<float>(edges.bottom)});
}

PatchCache::PatchCache(std::size_t budgetBytes) : budget_(budgetBytes) {}

const NinePatch *PatchCache::find(const PatchKey &key) {
  const auto found = entries_.find(key);
  if (found == entries_.end()) return nullptr;
  uses_.splice(uses_.begin(), uses_, found->second.use);
  return &found->second.patch;
}

const NinePatch &PatchCache::insert(PatchKey key, NinePatch patch) {
  if (const auto found = entries_.find(key); found != entries_.end()) erase(found);
  bytes_ += bytesOf(patch);
  const auto entry = entries_.try_emplace(std::move(key), Entry{.patch = std::move(patch), .use = {}}).first;
  uses_.push_front(&entry->first);
  entry->second.use = uses_.begin();
  while (bytes_ > budget_ && uses_.size() > 1) erase(entries_.find(*uses_.back()));
  return entry->second.patch;
}

std::size_t PatchCache::bytes() const {
  return bytes_;
}

void PatchCache::erase(Entries::iterator entry) {
  bytes_ -= bytesOf(entry->second.patch);
  uses_.erase(entry->second.use);
  entries_.erase(entry);
}

PatchCache &patchCache() {
  static PatchCache shared(kSharedBudgetBytes);
  return shared;
}

} // namespace rntizen
