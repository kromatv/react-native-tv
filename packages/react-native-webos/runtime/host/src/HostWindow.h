#pragma once

#include <Evas.h>

#include <string>

namespace rntizen {

/// Opens the app's Elementary window, black and `width` by `height`, asks for
/// the GPU, and logs the rendering engine and fonts it got. Main thread only.
Evas_Object *createHostWindow(const std::string &title, int width, int height);

/// The window createHostWindow opened, or null before it and once it is deleted. Main thread only.
Evas_Object *hostWindow();

/// Turns the window's alpha channel on, so a video plane under the window shows
/// through the transparent holes video views cut. The black background stays,
/// so nothing else under the window, the home screen included, ever shows.
/// Main thread only.
void setHostWindowTransparent(bool transparent);

} // namespace rntizen
