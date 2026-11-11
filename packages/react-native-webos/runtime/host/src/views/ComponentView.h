#pragma once

#include "../render/Scene.h"
#include "BoxPainter.h"
#include "PixelGeometry.h"
#include "ViewTransform.h"

#include <folly/dynamic.h>
#include <react/renderer/core/EventEmitter.h>
#include <react/renderer/core/LayoutMetrics.h>
#include <react/renderer/core/Props.h>
#include <react/renderer/core/ReactPrimitives.h>
#include <react/renderer/core/State.h>

#include <span>
#include <string>
#include <vector>

namespace rnwebos {

/// The native side of one mounted React Native view: its box (shadows,
/// background, border, clip), what it draws and its children, drawn with Skia
/// when the scene draws a frame. Layout arrives in points; every view places
/// itself in whole device pixels at the surface's point scale factor.
/// Subclasses add what a component draws. Main thread only.
class ComponentView : public SceneLayer {
 public:
  ComponentView(Scene &scene, facebook::react::Tag tag);
  ~ComponentView() override;

  ComponentView(const ComponentView &) = delete;
  ComponentView &operator=(const ComponentView &) = delete;

  facebook::react::Tag tag() const;
  ComponentView *parent() const;
  std::span<ComponentView *const> children() const;
  const facebook::react::LayoutMetrics &layoutMetrics() const;
  const facebook::react::Props::Shared &props() const;
  const facebook::react::EventEmitter::Shared &eventEmitter() const;

  virtual void updateProps(const facebook::react::Props::Shared &props);
  virtual void updateState(const facebook::react::State::Shared &state);
  virtual void updateLayout(const facebook::react::LayoutMetrics &layoutMetrics);
  void updateEventEmitter(const facebook::react::EventEmitter::Shared &eventEmitter);

  /// Runs a view command such as ScrollView's `scrollTo`. A view drops the
  /// commands it does not implement, as `focus` or `setDestinations`.
  virtual void handleCommand(const std::string &name, const folly::dynamic &args);

  void mountChild(ComponentView &child, std::size_t index);
  void unmountChild(ComponentView &child);

  /// Shows the view as a surface's root, a layer of the scene.
  void mountAsRoot(int order);

  /// Draws the view and its subtree where its parent's canvas puts it.
  void draw(SkCanvas &canvas) override;

  /// The view's box in the window, in pixels, before any transform.
  PixelRect canvasRect() const;
  /// The view's top-left corner in the window, in pixels.
  facebook::react::Point canvasOrigin() const;
  /// The box as it shows on screen, in pixels: `canvasRect()` through the
  /// view's own transform and every ancestor's, as the rect around it.
  PixelRect screenRect() const;

  /// The deepest view under the window point `point` that takes pointer
  /// events, or null. `local` gets the point in that view's own pixels.
  ComponentView *hitTest(SkPoint point, SkPoint *local = nullptr);

 protected:
  Scene &scene() const;
  /// The size of the view's box, in pixels.
  PixelSize pixelSize() const;
  /// Device pixels per layout point.
  facebook::react::Float pointScale() const;
  /// Asks for a redraw: call it when what the view draws changed.
  void invalidate();

  /// Draws what the view makes of the pixels already behind it, in its own
  /// pixels and at `opacity`: before the layer its own opacity may open, which
  /// starts empty and would leave a backdrop filter nothing to read.
  virtual void drawBackdrop(SkCanvas &canvas, float opacity);
  /// Draws what the component shows inside its box, under its children, in
  /// the view's own pixels; an `alpha` under 1 comes only to a view whose
  /// `contentLayers()` is 1, which then draws at that opacity.
  virtual void drawContent(SkCanvas &canvas, float alpha);
  /// Draws the children; a view that masks or filters them overrides it.
  virtual void drawChildren(SkCanvas &canvas);
  /// Draws `child` where the view's canvas puts it, unless it is an overlay,
  /// which the scene draws.
  static void drawChild(SkCanvas &canvas, ComponentView &child);
  /// Draws over the children and the border, in the view's own pixels.
  virtual void drawOverChildren(SkCanvas &canvas);
  /// How many things `drawContent` paints on top of each other: 0 for none.
  virtual int contentLayers() const;
  /// Runs once the box moved or changed size.
  virtual void layoutContent();
  /// How far the children sit from their layout position, in points: minus a
  /// scroll view's content offset.
  virtual facebook::react::Point childrenOffset() const;
  /// Clips content and children to the box even when `overflow` is visible.
  void setAlwaysClips(bool alwaysClips);
  /// Whether the view is drawn above every other view, at the screen's
  /// origin, rather than inside its parent (a Modal).
  virtual bool isOverlay() const;
  /// Runs once the view joined a parent.
  virtual void didMount();
  /// Runs before the view leaves its parent, while `parent()` is still set.
  /// A view destroyed while mounted gets no call: its destructor runs instead.
  virtual void willUnmount();
  /// Runs once a child joined or left.
  virtual void didChangeChildren();

 private:
  void place();
  void refreshBox();
  PixelPoint childrenShift() const;
  SkRect paintBounds() const;
  bool hitTestLocal(SkPoint point, ComponentView *&hit, SkPoint *local);

  Scene &scene_;
  const facebook::react::Tag tag_;
  BoxPainter box_;
  ViewTransform transform_;
  ComponentView *parent_{nullptr};
  std::vector<ComponentView *> children_;
  facebook::react::LayoutMetrics layoutMetrics_{facebook::react::EmptyLayoutMetrics};
  facebook::react::Props::Shared props_;
  facebook::react::EventEmitter::Shared eventEmitter_;
  PixelPoint origin_;
  PixelSize size_;
  float opacity_{1};
  bool alwaysClips_{false};
  bool isRoot_{false};
  bool isLayer_{false};
};

} // namespace rnwebos
