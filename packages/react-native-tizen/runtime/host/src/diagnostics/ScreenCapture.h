#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace rntizen {

/// The window as Evas drew it: premultiplied BGRA bytes, top row first, rows `stride` bytes apart.
struct ScreenCapture {
  int width;
  int height;
  int stride;
  std::vector<std::uint8_t> pixels;
};

using CaptureDone = std::function<void(std::optional<ScreenCapture>)>;

/// Captures what Evas draws in the host window at its next frame, through a
/// snapshot image over the whole canvas, then calls `done` on the main thread;
/// with nothing when there is no window or Evas gives no pixels. The hardware
/// video plane and the system's own UI are never part of it. Main thread only.
void captureHostWindow(CaptureDone done);

} // namespace rntizen
