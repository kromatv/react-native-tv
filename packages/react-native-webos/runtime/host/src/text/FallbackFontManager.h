#pragma once

#include "include/core/SkFontMgr.h"

namespace rnwebos {

/// `fonts` with a fallback for characters a family lacks: the first family
/// that has the character, at the closest style. Skia's directory font
/// manager has none of its own (docs/reference/skia-rendering.md, section 6),
/// and without one a Korean or an emoji character in a Latin font draws as a
/// box. Any thread.
sk_sp<SkFontMgr> withCharacterFallback(sk_sp<SkFontMgr> fonts);

} // namespace rnwebos
