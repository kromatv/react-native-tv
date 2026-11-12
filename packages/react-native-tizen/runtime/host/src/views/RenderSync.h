#pragma once

#include <Evas.h>

namespace rntizen {

/// Waits until the software engine's render thread has drawn its frame. Evas
/// frees an object's offscreen surface (a map's, a snapshot's, a proxy's) as
/// soon as the object is deleted, its map turned off or its proxy source
/// changed, while that thread may still be drawing into it
/// (efl-rendering.md section 7). Call this first. Returns at once when the
/// canvas renders on the main loop, as the GL engine does.
inline void waitForRenderThread(Evas_Object *object) {
  evas_sync(evas_object_evas_get(object));
}

} // namespace rntizen
