#include "Scene.h"

#include "../platform/MainLoop.h"

#include <algorithm>
#include <utility>

namespace rnwebos {

Scene::Scene(HostWindow &window) : window_(&window), size_(window.size()) {
  MainLoop::get().setRenderer([this] { return render(); });
  MainLoop::get().requestFrame();
}

Scene::Scene(PixelSize size) : window_(nullptr), size_(size) {}

Scene::~Scene() {
  if (window_ != nullptr) MainLoop::get().setRenderer(nullptr);
}

HostWindow *Scene::window() const {
  return window_;
}

PixelSize Scene::size() const {
  return size_;
}

void Scene::invalidate() {
  if (std::exchange(invalid_, true)) return;
  if (window_ != nullptr) MainLoop::get().requestFrame();
}

void Scene::addLayer(SceneLayer &layer, int order) {
  const auto above = std::ranges::find_if(layers_, [order](const Entry &entry) { return entry.order > order; });
  layers_.insert(above, {.layer = &layer, .order = order});
  invalidate();
}

void Scene::removeLayer(SceneLayer &layer) {
  if (std::erase_if(layers_, [&layer](const Entry &entry) { return entry.layer == &layer; }) > 0) invalidate();
}

void Scene::setTransparent(bool transparent) {
  if (std::exchange(transparent_, transparent) != transparent) invalidate();
}

void Scene::setFrameObserver(std::function<void(const FrameTiming &)> observer) {
  frameObserver_ = std::move(observer);
}

void Scene::drawInto(SkCanvas &canvas) {
  canvas.clear(transparent_ ? SK_ColorTRANSPARENT : SK_ColorBLACK);
  const auto layers = layers_;
  for (const auto &entry : layers) entry.layer->draw(canvas);
}

bool Scene::render() {
  if (window_ == nullptr || !std::exchange(invalid_, false)) return false;
  const auto started = std::chrono::steady_clock::now();
  drawInto(window_->canvas());
  const auto drawn = std::chrono::steady_clock::now();
  window_->present();
  if (frameObserver_) {
    frameObserver_({.started = started, .draw = drawn - started, .present = std::chrono::steady_clock::now() - drawn});
  }
  return true;
}

} // namespace rnwebos
