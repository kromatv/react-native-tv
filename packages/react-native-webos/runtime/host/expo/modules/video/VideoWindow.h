#pragma once

#include "render/Scene.h"

namespace rnwebos::expo {

/// Counts the views of `scene` showing a video. While one does, the scene is
/// cleared to transparent, so the video plane also shows through a hole that
/// sits in a layer, such as under an ancestor with an opacity
/// (docs/reference/webos-media-playback.md, section 2). Main thread only.
void videoViewShown(Scene &scene, bool shown);

} // namespace rnwebos::expo
