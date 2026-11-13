#pragma once

#include <Evas.h>

#include <functional>

namespace rntizen::expo {

/// Runs `then` once the canvas of `window` has drawn a frame it began after
/// this call; after a tenth of a second when it draws none, as an app in the
/// background does; at once when `window` is null, and when it is deleted
/// first. Main thread only.
void afterNextFrame(Evas_Object *window, std::function<void()> then);

} // namespace rntizen::expo
