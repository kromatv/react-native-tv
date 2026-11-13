#include "VideoHoles.h"

#include <doctest/doctest.h>

#include <initializer_list>

namespace rntizen::expo {

using enum HoleLook;

TEST_CASE("a view without a player on the overlay covers nothing") {
  CHECK(videoHoles({}) == VideoHoles{.inView = Hidden, .onTop = Hidden});
  CHECK(videoHoles({.showsPlayer = false, .hasPicture = true, .fullscreen = true, .mapped = true}) ==
        VideoHoles{.inView = Hidden, .onTop = Hidden});
}

TEST_CASE("a view is opaque black over its box until the plane has a picture, and clear while it has one") {
  CHECK(videoHoles({.showsPlayer = true}) == VideoHoles{.inView = Black, .onTop = Hidden});
  CHECK(videoHoles({.showsPlayer = true, .hasPicture = true}) == VideoHoles{.inView = Clear, .onTop = Hidden});
}

TEST_CASE("a view closes to black again when the plane stops having a picture") {
  VideoViewState view{.showsPlayer = true, .hasPicture = true};
  REQUIRE(videoHoles(view).inView == Clear);
  view.hasPicture = false;
  CHECK(videoHoles(view) == VideoHoles{.inView = Black, .onTop = Hidden});
  view.showsPlayer = false;
  CHECK(videoHoles(view) == VideoHoles{.inView = Hidden, .onTop = Hidden});
}

TEST_CASE("a fullscreen view covers the window from above every view, black until the picture") {
  CHECK(videoHoles({.showsPlayer = true, .fullscreen = true}) == VideoHoles{.inView = Hidden, .onTop = Black});
  CHECK(videoHoles({.showsPlayer = true, .hasPicture = true, .fullscreen = true}) ==
        VideoHoles{.inView = Hidden, .onTop = Clear});
  CHECK(videoHoles({.showsPlayer = true, .hasPicture = true, .fullscreen = true, .mapped = true}) ==
        VideoHoles{.inView = Hidden, .onTop = Clear});
}

TEST_CASE("a mapped view is black inside its map and clear only from above it") {
  CHECK(videoHoles({.showsPlayer = true, .mapped = true}) == VideoHoles{.inView = Black, .onTop = Hidden});
  CHECK(videoHoles({.showsPlayer = true, .hasPicture = true, .mapped = true}) ==
        VideoHoles{.inView = Hidden, .onTop = Clear});
}

TEST_CASE("no view state shows a clear hole without a picture") {
  for (const bool showsPlayer : {false, true}) {
    for (const bool fullscreen : {false, true}) {
      for (const bool mapped : {false, true}) {
        const auto holes = videoHoles({.showsPlayer = showsPlayer, .fullscreen = fullscreen, .mapped = mapped});
        CHECK(holes.inView != Clear);
        CHECK(holes.onTop != Clear);
      }
    }
  }
}

} // namespace rntizen::expo
