#pragma once

#include "PixelGeometry.h"
#include "ViewBox.h"
#include "ViewTransform.h"

#include <folly/dynamic.h>
#include <react/renderer/core/EventEmitter.h>
#include <react/renderer/core/LayoutMetrics.h>
#include <react/renderer/core/Props.h>
#include <react/renderer/core/ReactPrimitives.h>
#include <react/renderer/core/State.h>

#include <Evas.h>

#include <string>
#include <vector>

namespace rntizen {

/// The native side of one mounted React Native view: an Evas smart object
/// holding its box (shadows, background, border, clip), what it draws and its
/// children. Layout arrives in points; every object is placed in canvas
/// pixels at the surface's point scale factor, by this class and not by Evas,
/// so a move reaches each descendant once. Subclasses add what a component
/// draws. Main thread only.
class ComponentView {
 public:
  ComponentView(Evas *evas, facebook::react::Tag tag);
  virtual ~ComponentView();

  ComponentView(const ComponentView &) = delete;
  ComponentView &operator=(const ComponentView &) = delete;

  facebook::react::Tag tag() const;
  ComponentView *parent() const;
  const std::vector<ComponentView *> &children() const;
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

  /// Places the view on the canvas as a surface's root. A view is placed only
  /// while a root holds it, so a subtree Fabric builds bottom-up is placed
  /// once, when it joins one, rather than at every level it is inserted at.
  void mountAsRoot();

  /// The view's top-left corner on the canvas, in pixels.
  facebook::react::Point canvasOrigin() const;

 protected:
  Evas *evas() const;
  Evas_Object *root() const;
  /// The view's box on the canvas, in pixels.
  const PixelRect &canvasRect() const;
  /// The box as it shows on screen, in pixels: `canvasRect()` through the
  /// view's own transform and every ancestor's, as the rect around it.
  PixelRect screenRect() const;
  /// Device pixels per layout point.
  facebook::react::Float pointScale() const;
  /// Adds an object the view draws. Below the children, it is clipped with
  /// them; otherwise it is drawn above everything, the border included.
  void addMember(Evas_Object *member, bool belowChildren);
  /// Places the members once the box moved or changed size.
  virtual void layoutMembers();
  /// How far the children sit from their layout position, in points: minus a
  /// scroll view's content offset.
  virtual facebook::react::Point childrenOffset() const;
  void placeChildren();
  /// Clips content and children to the box even when `overflow` is visible.
  void setAlwaysClips(bool alwaysClips);
  /// Whether the view is drawn above every other view, at the screen's
  /// origin, rather than inside its parent (a Modal).
  virtual bool isOverlay() const;
  /// Runs once the view joined a parent and was placed.
  virtual void didMount();
  /// Runs before the view leaves its parent, while `parent()` is still set.
  /// A view destroyed while mounted gets no call: its destructor runs instead.
  virtual void willUnmount();
  /// What `child` is clipped to instead of the box, or null for the box (a
  /// MaskedView's content hangs from its mask).
  virtual Evas_Object *clipperFor(const ComponentView &child) const;
  /// Runs once a child joined or left.
  virtual void didChangeChildren();
  /// Hangs every member and child from its clipper again, after
  /// `clipperFor` changed its answer.
  void refreshClips();
  /// The smart object holding `view` and everything it draws.
  static Evas_Object *rootOf(const ComponentView &view);

 private:
  struct SmartObject {
    Evas_Object *const object;
    ~SmartObject();
  };

  void place();
  void setRooted(bool rooted);
  void refreshBox();
  void restack();
  void stackChild(std::size_t index);
  void hangFromClipper(Evas_Object *object) const;
  void hangChild(const ComponentView &child) const;
  void attach(ComponentView &child);
  void detach(ComponentView &child);
  void applyVisibility();
  PixelPoint childrenOrigin() const;
  PixelRect bounds() const;

  Evas *const evas_;
  const facebook::react::Tag tag_;
  SmartObject smart_;
  Evas_Object *const root_;
  ViewBox box_;
  ViewTransform transform_;
  ComponentView *parent_{nullptr};
  std::vector<ComponentView *> children_;
  std::vector<Evas_Object *> contents_;
  std::vector<Evas_Object *> overlays_;
  facebook::react::LayoutMetrics layoutMetrics_{facebook::react::EmptyLayoutMetrics};
  facebook::react::Props::Shared props_;
  facebook::react::EventEmitter::Shared eventEmitter_;
  PixelRect rect_;
  bool placed_{false};
  bool rooted_{false};
  bool detached_{false};
  bool alwaysClips_{false};
};

} // namespace rntizen
