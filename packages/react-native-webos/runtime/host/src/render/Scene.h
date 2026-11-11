#pragma once

#include "../platform/HostWindow.h"

#include <chrono>
#include <functional>
#include <vector>

namespace rnwebos {

/// Something the window shows: a surface's view tree, a Modal, the error screen.
class SceneLayer {
 public:
  virtual ~SceneLayer() = default;
  /// Draws the layer in window pixels.
  virtual void draw(SkCanvas &canvas) = 0;
};

/// What one drawn frame cost, for `rn-webos perf`.
struct FrameTiming {
  std::chrono::steady_clock::time_point started;
  /// Walking the views and recording Skia's draws.
  std::chrono::nanoseconds draw;
  /// Handing the draws to the GPU.
  std::chrono::nanoseconds flush;
  /// Swapping, which waits for the display where it paces the swap.
  std::chrono::nanoseconds present;
};

/// Everything the window shows, as layers drawn lowest order first. Nothing is
/// drawn until something invalidates the scene; a frame then redraws it whole
/// (docs/reference/skia-rendering.md). Main thread only.
class Scene final {
 public:
  /// Layer orders: a surface's views, a Modal, then the host's own overlays.
  static constexpr int kSurfaceOrder = 0;
  static constexpr int kModalOrder = 1000;
  static constexpr int kDialogOrder = 2000;
  static constexpr int kBannerOrder = 2500;
  static constexpr int kErrorOrder = 3000;

  /// A scene shown in `window`, drawn by the main loop when it is invalid.
  explicit Scene(HostWindow &window);
  /// A scene of `size` pixels with no window, which draws only into the
  /// canvas `drawInto` is given: what the host's tests mount views in.
  explicit Scene(PixelSize size);
  ~Scene();

  Scene(const Scene &) = delete;
  Scene &operator=(const Scene &) = delete;

  /// The window the scene is shown in, or null for a scene without one.
  HostWindow *window() const;
  PixelSize size() const;

  /// Something changed: the scene is drawn at the next frame.
  void invalidate();
  /// Whether something changed since the last `render`.
  bool isInvalid() const;

  /// Adds `layer` above every layer of the same or a lower order.
  void addLayer(SceneLayer &layer, int order);
  void removeLayer(SceneLayer &layer);

  /// Clears to transparent instead of black, so the video plane under the
  /// graphics plane shows where nothing is drawn.
  void setTransparent(bool transparent);

  /// Draws and presents the scene when it is invalid. True when it presented.
  bool render();
  /// Draws the scene into `canvas`, as a screenshot or a test wants it.
  void drawInto(SkCanvas &canvas);

  void setFrameObserver(std::function<void(const FrameTiming &)> observer);

 private:
  struct Entry {
    SceneLayer *layer;
    int order;
  };

  HostWindow *const window_;
  const PixelSize size_;
  std::vector<Entry> layers_;
  std::function<void(const FrameTiming &)> frameObserver_;
  bool invalid_{true};
  bool transparent_{false};
};

} // namespace rnwebos
