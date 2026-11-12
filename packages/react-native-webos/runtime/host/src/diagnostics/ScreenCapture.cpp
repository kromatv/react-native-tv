#include "ScreenCapture.h"

#include "../logging/Log.h"

#include "include/core/SkBitmap.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkSurface.h"

#include <bit>

namespace rnwebos {

namespace {

static_assert(std::endian::native == std::endian::little, "Skia's BGRA_8888 pixels must read as BGRA bytes");

SkImageInfo wireFormat(PixelSize size) {
  return SkImageInfo::Make(size.width, size.height, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
}

ScreenCapture emptyCapture(PixelSize size) {
  const auto rowBytes = static_cast<std::size_t>(size.width) * 4;
  return {
      .width = size.width,
      .height = size.height,
      .stride = static_cast<int>(rowBytes),
      .pixels = std::vector<std::uint8_t>(rowBytes * static_cast<std::size_t>(size.height)),
  };
}

// eglSwapBuffers leaves the back buffer undefined unless the surface preserves
// it, so the frame is drawn again before it is read, then once more to be
// shown (docs/reference/react-native-dev-tooling.md, section 11).
std::optional<ScreenCapture> readWindow(Scene &scene, HostWindow &window) {
  SkBitmap frame;
  scene.drawInto(window.canvas());
  const bool read = window.readPixels(frame);
  scene.invalidate();
  if (!read) return std::nullopt;
  auto capture = emptyCapture({frame.width(), frame.height()});
  const auto format = wireFormat({frame.width(), frame.height()});
  if (!frame.readPixels(format, capture.pixels.data(), static_cast<std::size_t>(capture.stride), 0, 0)) return std::nullopt;
  return capture;
}

std::optional<ScreenCapture> drawOnCpu(Scene &scene) {
  auto capture = emptyCapture(scene.size());
  const auto surface =
      SkSurfaces::WrapPixels(wireFormat(scene.size()), capture.pixels.data(), static_cast<std::size_t>(capture.stride));
  if (!surface) return std::nullopt;
  scene.drawInto(*surface->getCanvas());
  return capture;
}

} // namespace

std::optional<ScreenCapture> captureScene(Scene &scene) {
  const auto size = scene.size();
  if (size.width <= 0 || size.height <= 0) return std::nullopt;
  if (auto *window = scene.window()) {
    if (auto capture = readWindow(scene, *window)) return capture;
    log(LogLevel::Debug, "The window's pixels cannot be read: drawing the screenshot on the CPU");
  }
  return drawOnCpu(scene);
}

} // namespace rnwebos
