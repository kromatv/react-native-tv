#pragma once

namespace rnwebos::expo {

/// Counts the views showing a video. While one does, the window turns
/// transparent, which with each view's hole is what lets the overlay plane show
/// through a page that has a background (`webos-media-playback.md`, section 3).
/// Main thread only.
void videoViewShown(bool shown);

} // namespace rnwebos::expo
