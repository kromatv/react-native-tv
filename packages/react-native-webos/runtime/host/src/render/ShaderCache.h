#pragma once

#include "include/core/SkData.h"
#include "include/core/SkString.h"
#include "include/gpu/ganesh/GrContextOptions.h"

#include <cstddef>
#include <string>

namespace rnwebos {

/// Skia's compiled shader programs, kept in a directory between runs. Without
/// it every program is compiled again at each start, in the frame that first
/// draws with it, which a TV's GPU takes long over
/// (docs/reference/skia-rendering.md, section 7). A file that is missing,
/// damaged or another program's is a miss. Main thread only.
class ShaderCache final : public GrContextOptions::PersistentCache {
 public:
  /// The programs kept in `directory`, which is created when missing. A
  /// directory that cannot be written keeps nothing.
  explicit ShaderCache(std::string directory);

  sk_sp<SkData> load(const SkData &key) override;
  void store(const SkData &key, const SkData &data, const SkString &description) override;

 private:
  std::string pathOf(const SkData &key) const;

  const std::string directory_;
  std::size_t storedBytes_{0};
};

} // namespace rnwebos
