#pragma once

#include "include/core/SkTypeface.h"

#include <vector>

namespace rnwebos {

/// What one font file gives a family: `typeface`, or, for a variable font, an
/// instance at each hundred its weight axis covers. SkParagraph picks a
/// typeface by the weight it reports, so one alone would draw every `fontWeight`.
std::vector<sk_sp<SkTypeface>> weightInstances(sk_sp<SkTypeface> typeface);

} // namespace rnwebos
