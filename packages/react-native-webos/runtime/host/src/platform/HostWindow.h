#pragma once

#include "../render/ShaderCache.h"
#include "../views/PixelGeometry.h"

#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"

#include <SDL3/SDL_video.h>

#include <cstddef>
#include <memory>
#include <string>

namespace rnwebos {

/// The app's one window: an SDL window over the TV's graphics plane, its
/// OpenGL ES context, and the Skia surface that draws into it
/// (docs/reference/webos-window-input.md, section 15). Main thread only.
class HostWindow final {
 public:
  /// Opens the window fullscreen; `size` is what it asks for where the
  /// display does not decide, and `shaderCacheDirectory` where Skia's compiled
  /// shaders are kept between runs. Throws std::runtime_error when the
  /// window, the context or the Skia surface cannot be made.
  HostWindow(const std::string &title, PixelSize size, const std::string &shaderCacheDirectory);
  ~HostWindow();

  HostWindow(const HostWindow &) = delete;
  HostWindow &operator=(const HostWindow &) = delete;

  /// The size of the drawing surface, in pixels.
  PixelSize size() const;
  /// The canvas of the frame being drawn.
  SkCanvas &canvas();
  GrDirectContext &gpu();
  SDL_Window *sdlWindow() const;

  /// Hands what was drawn to the GPU, without showing it.
  void flush();
  /// Sends what was drawn to the display, and waits for it where the display
  /// paces the swap.
  void present();

  /// Frees the GPU memory Skia keeps to draw the next frame faster, which it
  /// makes again when asked to draw: for an app the TV has hidden. Returns the
  /// bytes freed, by Skia's count.
  std::size_t releaseGpuMemory();

  /// Copies the last drawn frame, top row first, into `pixels`. False when
  /// the driver refuses the read.
  bool readPixels(SkBitmap &pixels);

  /// The display's refresh rate in hertz, or 0 when SDL does not know it.
  float refreshRate() const;

 private:
  void createContext();
  void createSurface();

  SDL_Window *window_{nullptr};
  SDL_GLContext context_{nullptr};
  std::unique_ptr<ShaderCache> shaderCache_;
  sk_sp<GrDirectContext> gpu_;
  sk_sp<SkSurface> surface_;
  PixelSize size_;
};

} // namespace rnwebos
