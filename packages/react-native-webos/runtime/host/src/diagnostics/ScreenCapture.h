#pragma once

#include "../render/Scene.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace rnwebos {

/// The window as the host drew it: premultiplied BGRA bytes, top row first, rows `stride` bytes apart.
struct ScreenCapture {
  int width;
  int height;
  int stride;
  std::vector<std::uint8_t> pixels;
};

using CaptureDone = std::function<void(std::optional<ScreenCapture>)>;

/// What `scene` shows, drawn now: read back from the window's framebuffer,
/// or drawn again on the CPU when the scene has no window or its driver
/// refuses the read. Nothing when neither gives pixels. The hardware video
/// plane and the system's own UI are never part of it. Main thread only.
std::optional<ScreenCapture> captureScene(Scene &scene);

} // namespace rnwebos
