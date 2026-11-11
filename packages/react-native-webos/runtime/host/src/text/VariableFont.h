#pragma once

#include "include/core/SkTypeface.h"

#include <vector>

namespace rnwebos {

/// What one font file gives a family: `typeface`, or, for a variable font with
/// a weight axis, an instance at each hundred the axis covers. SkParagraph
/// picks a typeface by the weight it reports and emboldens a lighter one, so
/// without the instances every `fontWeight` draws the default one.
std::vector<sk_sp<SkTypeface>> weightInstances(sk_sp<SkTypeface> typeface);

} // namespace rnwebos
