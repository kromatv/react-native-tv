#include "MediaPlayerHarness.test.h"

#include "render/Scene.h"

#include <doctest/doctest.h>

namespace rnwebos::expo {

using testing::Fixture;
using testing::Lines;
using testing::media;

TEST_CASE("the plane goes where the view is, fitted to the video, and the first playing frame is told once") {
  Fixture fixture;
  Scene scene(PixelSize{1920, 1080});
  int firstFrames = 0;
  int displayChanges = 0;
  const VideoView *view = nullptr;
  const auto owner = [&view] { return reinterpret_cast<const VideoView *>(&view); };
  const auto presentation = [&](ContentFit fit) {
    return Presentation{
        .scene = &scene,
        .view = {.x = 100, .y = 100, .width = 400, .height = 400},
        .window = {.x = 0, .y = 0, .width = 1920, .height = 1080},
        .fit = fit,
        .onFirstFrame = [&firstFrames] { ++firstFrames; },
        .onDisplayChange = [&displayChanges] { ++displayChanges; },
    };
  };
  fixture.player.open("https://example.com/a.mp4", {});
  fixture.calls.clear();
  fixture.player.present(owner(), presentation(ContentFit::Contain));
  CHECK(fixture.takeCalls() == Lines{"show 0,0 0x0 at 100,100 400x400"});
  fixture.player.prepare();
  fixture.backend->listener.backendLoaded(media(100, 1600, 800));
  CHECK(fixture.takeCalls() == Lines{"load https://example.com/a.mp4 max 0x0 prebuffer 0", "show 0,0 1600x800 at 100,200 400x200"});
  fixture.player.present(owner(), presentation(ContentFit::Contain));
  CHECK(fixture.calls.empty());
  fixture.player.present(owner(), presentation(ContentFit::Cover));
  CHECK(fixture.takeCalls() == Lines{"show 400,0 800x800 at 100,100 400x400"});
  fixture.player.present(owner(), presentation(ContentFit::Fill));
  CHECK(fixture.takeCalls() == Lines{"show 0,0 1600x800 at 100,100 400x400"});
  fixture.backend->listener.backendVideoChanged({.width = 800, .height = 800, .fps = 25, .bitrate = 1, .codec = "h264"});
  CHECK(fixture.takeCalls() == Lines{"show 0,0 800x800 at 100,100 400x400"});

  CHECK(firstFrames == 0);
  fixture.player.play();
  fixture.backend->listener.backendPlayingChanged(true);
  fixture.backend->listener.backendPlayingChanged(false);
  fixture.backend->listener.backendPlayingChanged(true);
  CHECK(firstFrames == 1);

  fixture.calls.clear();
  fixture.player.withdraw(nullptr);
  CHECK(fixture.calls.empty());
  fixture.player.withdraw(owner());
  fixture.player.withdraw(owner());
  CHECK(fixture.takeCalls() == Lines{"hide"});

  fixture.player.present(owner(), presentation(ContentFit::Fill));
  CHECK(fixture.player.hasDisplay());
  fixture.calls.clear();
  fixture.player.release();
  fixture.player.release();
  CHECK(fixture.calls == Lines{"unload", "hide"});
  CHECK_FALSE(fixture.player.hasDisplay());
  CHECK(displayChanges == 1);
  fixture.player.open("https://example.com/b.mp4", {});
  fixture.player.play();
  CHECK(fixture.calls == Lines{"unload", "hide"});
}

} // namespace rnwebos::expo
