#pragma once

#include <react/renderer/attributedstring/AttributedString.h>
#include <react/renderer/attributedstring/ParagraphAttributes.h>

#include <optional>
#include <string>

namespace rntizen {

/// An Evas textblock's style and markup for one paragraph. The measurer and
/// the paragraph view both build theirs here, so layout and pixels agree.
/// `tightLineShift` is set when a run's `lineHeight` is below its font's own
/// line (see `FragmentFormat` in TextblockFormat.h): the paragraph then spans
/// its first line's top to its last line's bottom, and its text sits that many
/// pixels lower.
struct TextblockContent {
  std::string style;
  std::string markup;
  std::optional<int> tightLineShift;
};

/// The paragraph at `scale` device pixels per point.
TextblockContent textblockContent(
    const facebook::react::AttributedString &attributedString,
    const facebook::react::ParagraphAttributes &paragraphAttributes,
    facebook::react::Float scale);

} // namespace rntizen
