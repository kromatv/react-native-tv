#pragma once

#include <Evas.h>

namespace rntizen {

/// A size in pixels.
struct PixelSize {
  int width;
  int height;
};

/// Sizes the app's window: to the screen the window manager reports when the
/// app asked for a 4K UI and the screen is 3840x2160 or larger, so the point
/// scale becomes 2; to `systemInfo` otherwise. Logs what each source reports
/// and the canvas it got. Main thread only, right after the window is created.
PixelSize fitWindowToScreen(Evas_Object *window, bool uhd, PixelSize systemInfo);

} // namespace rntizen
