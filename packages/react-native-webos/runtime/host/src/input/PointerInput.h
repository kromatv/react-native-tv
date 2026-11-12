#pragma once

#include "../render/Scene.h"

#include <react/renderer/core/ReactPrimitives.h>

#include <SDL3/SDL_events.h>

#include <functional>
#include <vector>

namespace rnwebos {

class ComponentView;

/// The remote's pointer (LG's Magic Remote), which reaches the app as a mouse
/// (docs/reference/webos-window-input.md, section 9). Moving it sends W3C
/// pointer enter, leave and move events to the views under it, which is what
/// `Pressable`'s hover callbacks read; its button sends touches, which React
/// Native's responder system turns into presses; its wheel scrolls the nearest
/// view that scrolls. Main thread only.
class PointerInput final {
 public:
  using ViewFinder = std::function<ComponentView *(facebook::react::Tag tag)>;

  /// `findView` resolves a mounted view, or null once it is gone.
  PointerInput(Scene &scene, ViewFinder findView);

  PointerInput(const PointerInput &) = delete;
  PointerInput &operator=(const PointerInput &) = delete;

  /// Takes a mouse event of the window; true when it was one.
  bool handleEvent(const SDL_Event &event);

  /// The pointer is gone: the remote went back to its arrow keys, or the app
  /// lost the focus. Ends the hover and cancels a press in progress.
  void pointerLeft();

 private:
  struct Target {
    ComponentView *view{nullptr};
    SkPoint local{};
  };

  Target targetAt(SkPoint point) const;
  void moveTo(SkPoint point);
  void press(SkPoint point);
  void release(SkPoint point, bool cancelled);
  void wheel(SkPoint point, float dx, float dy);
  void hover(const Target &target, SkPoint point);

  Scene &scene_;
  const ViewFinder findView_;
  std::vector<facebook::react::Tag> hovered_;
  facebook::react::Tag pressed_{0};
  SkPoint position_{};
};

} // namespace rnwebos
