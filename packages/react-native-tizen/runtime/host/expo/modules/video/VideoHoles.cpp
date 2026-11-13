#include "VideoHoles.h"

namespace rntizen::expo {

VideoHoles videoHoles(const VideoViewState &view) {
  using enum HoleLook;
  if (!view.showsPlayer) return {};
  const auto look = view.hasPicture ? Clear : Black;
  // Evas renders a mapped subtree into a surface first, so a clear hole inside
  // one cuts nothing (tizen-media-playback.md, section 3); black draws there as any object.
  const bool above = view.fullscreen || (view.mapped && look == Clear);
  return above ? VideoHoles{.inView = Hidden, .onTop = look} : VideoHoles{.inView = look, .onTop = Hidden};
}

} // namespace rntizen::expo
