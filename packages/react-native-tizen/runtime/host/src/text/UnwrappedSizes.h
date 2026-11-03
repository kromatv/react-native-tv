#pragma once

#include <react/renderer/textlayoutmanager/TextMeasureCache.h>

#include <mutex>
#include <optional>
#include <unordered_map>

namespace rntizen {

/// The sizes of paragraphs Evas laid out without wrapping a line or cutting one
/// with an ellipsis, by content, paragraph attributes and point scale. Evas
/// wraps a line only once its text passes the width (efl-rendering.md section
/// 5), so the paragraph lays out the same at any width at least as wide as its
/// widest line: Yoga measuring a label at its content width and then at its
/// parent's costs one layout. Safe from any thread.
class UnwrappedSizes final {
 public:
  using Key = facebook::react::TextMeasureCacheKey;

  /// The size `key` takes at `widthPixels`, if it fits there unwrapped.
  std::optional<facebook::react::Size> find(const Key &key, int widthPixels) const;

  /// Records that `key` laid out unwrapped as `size`, its widest line `widthPixels` wide.
  void remember(const Key &key, int widthPixels, facebook::react::Size size) const;

 private:
  struct Entry {
    int widthPixels;
    facebook::react::Size size;
  };

  mutable std::mutex mutex_;
  mutable std::unordered_map<Key, Entry> entries_;
};

} // namespace rntizen
