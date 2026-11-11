#pragma once

namespace rntizen::expo {

/// Counts the views showing a video. While one does, the window turns
/// transparent, which with each view's hole is what lets the overlay plane show
/// through a page that has a background (`tizen-media-playback.md`, section 3).
/// Main thread only.
void videoViewShown(bool shown);

} // namespace rntizen::expo
