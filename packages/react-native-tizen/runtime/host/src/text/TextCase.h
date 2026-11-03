#pragma once

#include <react/renderer/attributedstring/primitives.h>

#include <string>

namespace rntizen {

/// `text` (UTF-8) in the case `textTransform` asks for. Covers ASCII, Latin-1
/// and Latin Extended-A, which French and the other Latin-script languages
/// need, without ICU (docs/decisions.md); other characters pass through.
std::string transformedText(const std::string &text, facebook::react::TextTransform textTransform);

} // namespace rntizen
