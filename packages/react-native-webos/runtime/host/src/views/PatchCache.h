#pragma once

#include "NinePatch.h"

#include <cstddef>
#include <list>
#include <map>
#include <vector>

namespace rntizen {

/// What a patch is painted from: its kind first, then every value its pixels
/// depend on.
using PatchKey = std::vector<float>;

/// The kinds of patches the shared cache holds, the first value of their key.
enum class PatchKind { Filled, Ring, Shadow, PatternedRing, Gradient };

/// A key for `kind` that starts with the eight radii of `corners`.
PatchKey patchKey(PatchKind kind, const Corners &corners);

/// Appends the four widths of `edges` to `key`.
void appendEdges(PatchKey &key, const Edges &edges);

/// Patches by key within a byte budget: the least recently used go first, the
/// newest always stays. A patch returned stays valid until the next `insert`.
class PatchCache final {
 public:
  explicit PatchCache(std::size_t budgetBytes);

  PatchCache(const PatchCache &) = delete;
  PatchCache &operator=(const PatchCache &) = delete;

  /// The patch under `key`, now the most recently used, or null.
  const NinePatch *find(const PatchKey &key);
  /// Stores `patch` under `key`, replacing any patch there.
  const NinePatch &insert(PatchKey key, NinePatch patch);
  /// The pixel bytes the cache holds.
  std::size_t bytes() const;

 private:
  using Uses = std::list<const PatchKey *>;
  struct Entry {
    NinePatch patch;
    Uses::iterator use;
  };
  using Entries = std::map<PatchKey, Entry>;

  void erase(Entries::iterator entry);

  const std::size_t budget_;
  std::size_t bytes_{0};
  Entries entries_;
  Uses uses_;
};

/// The cache every box, shadow and gradient patch shares. Main thread only.
PatchCache &patchCache();

/// The patch under `key` in the shared cache, painted by `paint` the first
/// time. Main thread only.
template <typename Paint>
const NinePatch &cachedPatch(const PatchKey &key, Paint paint) {
  auto &cache = patchCache();
  if (const auto *found = cache.find(key)) return *found;
  return cache.insert(key, paint());
}

} // namespace rntizen
