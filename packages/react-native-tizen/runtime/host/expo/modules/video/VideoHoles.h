#pragma once

namespace rntizen::expo {

/// How a video view covers the plane under it: not at all, with opaque black,
/// or with the transparent hole the plane shows through.
enum class HoleLook { Hidden, Black, Clear };

struct VideoViewState {
  /// The view has a box on screen and a player bound to the overlay.
  bool showsPlayer{false};
  /// That player's plane shows a picture.
  bool hasPicture{false};
  bool fullscreen{false};
  /// An ancestor maps the view, as a scale or rotate transform does.
  bool mapped{false};
};

/// What a video view draws over its own box, and above every view over its
/// on-screen rect or, in fullscreen, over the window.
struct VideoHoles {
  HoleLook inView{HoleLook::Hidden};
  HoleLook onTop{HoleLook::Hidden};

  bool operator==(const VideoHoles &) const = default;
};

/// Clear only while the plane shows a picture: with window alpha on, a clear
/// hole over a plane without one shows what is under the window, on a TV
/// Samsung's home screen (`tizen-media-playback.md`, section 3).
VideoHoles videoHoles(const VideoViewState &view);

} // namespace rntizen::expo
