#pragma once

#include <react/renderer/attributedstring/AttributedStringBox.h>
#include <react/renderer/attributedstring/ParagraphAttributes.h>
#include <react/renderer/core/LayoutConstraints.h>
#include <react/renderer/textlayoutmanager/TextLayoutContext.h>
#include <react/renderer/textlayoutmanager/TextMeasureCache.h>
#include <react/utils/ContextContainer.h>
#include <react/utils/SimpleThreadSafeCache.h>

#include <memory>

namespace facebook::react {

class PreparedText;
class TextMeasurer;

/// Fabric's text layout entry point on webOS: caches measurements and hands the
/// work to the host's TextMeasurer.
class TextLayoutManager {
 public:
  /// Declaring this type, `prepareLayout` and `measurePreparedLayout` is what
  /// makes ParagraphShadowNode lay a paragraph out once and hand the result to
  /// its view (docs/reference/react-native-platform-contract.md, Text layout).
  using PreparedTextLayout = std::shared_ptr<const PreparedText>;

  explicit TextLayoutManager(const std::shared_ptr<const ContextContainer> &contextContainer);
  virtual ~TextLayoutManager() = default;

  TextLayoutManager(const TextLayoutManager &) = delete;
  TextLayoutManager &operator=(const TextLayoutManager &) = delete;
  TextLayoutManager(TextLayoutManager &&) = delete;
  TextLayoutManager &operator=(TextLayoutManager &&) = delete;

  virtual TextMeasurement measure(
      const AttributedStringBox &attributedStringBox,
      const ParagraphAttributes &paragraphAttributes,
      const TextLayoutContext &layoutContext,
      const LayoutConstraints &layoutConstraints) const;

  PreparedTextLayout prepareLayout(
      const AttributedString &attributedString,
      const ParagraphAttributes &paragraphAttributes,
      const TextLayoutContext &layoutContext,
      const LayoutConstraints &layoutConstraints) const;

  TextMeasurement measurePreparedLayout(
      const PreparedTextLayout &layout,
      const TextLayoutContext &layoutContext,
      const LayoutConstraints &layoutConstraints) const;

 private:
  static constexpr int kPreparedTextCacheSize = 256;

  std::shared_ptr<const ContextContainer> contextContainer_;
  TextMeasureCache textMeasureCache_{kSimpleThreadSafeCacheSizeCap};
  SimpleThreadSafeCache<PreparedTextCacheKey, PreparedTextLayout, kPreparedTextCacheSize> preparedTextCache_;
  std::shared_ptr<const TextMeasurer> measurer_;
};

} // namespace facebook::react
