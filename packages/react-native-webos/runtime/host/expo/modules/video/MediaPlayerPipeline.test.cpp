#include "MediaPlayerHarness.test.h"

#include "render/Scene.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>

namespace rnwebos::expo {

using testing::Fixture;
using testing::Lines;
using testing::media;

namespace {

constexpr auto kSecondLoad = "load https://example.com/b.mp4 max 0x0 prebuffer 0";

/// A second player on the main loop of a Fixture.
struct Other {
  Lines calls;
  testing::ScriptedBackend *backend{nullptr};
  std::shared_ptr<testing::RecordingObserver> observer = std::make_shared<testing::RecordingObserver>();
  MediaPlayer player{observer, 2, [this](PlayerBackendListener &listener, int) {
                       auto made = std::make_unique<testing::ScriptedBackend>(listener, calls);
                       backend = made.get();
                       return made;
                     }};

  void open() {
    player.open("https://example.com/b.mp4", {});
    player.prepare();
  }
};

struct Screen {
  Scene scene{PixelSize{1920, 1080}};
  std::array<int, 2> views{};

  const VideoView *view(int index) const {
    return reinterpret_cast<const VideoView *>(&views[index]);
  }

  Presentation presentation() {
    return {
        .scene = &scene,
        .view = {.x = 0, .y = 0, .width = 1920, .height = 1080},
        .window = {.x = 0, .y = 0, .width = 1920, .height = 1080},
        .fit = ContentFit::Contain,
        .onFirstFrame = {},
        .onDisplayChange = {},
    };
  }
};

bool has(const Lines &lines, const std::string &line) {
  return std::ranges::find(lines, line) != lines.end();
}

} // namespace

TEST_CASE("a new player waits while a view shows another, and loads after that one unloaded once the view shows it") {
  Fixture first;
  Screen screen;
  first.openReady();
  first.player.present(screen.view(0), screen.presentation());
  first.player.play();
  first.backend->listener.backendPlayingChanged(true);
  first.calls.clear();
  first.observer->lines.clear();

  Other second;
  second.open();
  second.player.play();
  CHECK(second.calls == Lines{"foreground"});
  CHECK(second.observer->lines == Lines{"status loading"});
  CHECK(first.calls.empty());

  first.player.withdraw(screen.view(0));
  second.player.present(screen.view(0), screen.presentation());
  CHECK(first.calls == Lines{"unload", "hide"});
  CHECK(first.observer->lines == Lines{"status loading", "not playing"});
  CHECK(has(second.calls, kSecondLoad));

  second.backend->listener.backendLoaded(media(60));
  CHECK(second.calls.back() == "play");
  first.calls.clear();
  first.player.release();
  CHECK(first.calls == Lines{"unload", "hide"});
  CHECK(std::ranges::count(second.calls, kSecondLoad) == 1);
}

TEST_CASE("a player that lost the pipeline opens its source again where it was, and plays on, once the other is gone") {
  Fixture first;
  Screen screen;
  first.openReady();
  first.player.present(screen.view(0), screen.presentation());
  first.player.play();
  first.backend->listener.backendPlayingChanged(true);
  first.backend->listener.backendPlayingChanged(false);
  first.backend->listener.backendPosition(42);
  first.player.pause();
  first.player.play();
  first.calls.clear();
  first.observer->lines.clear();

  Other second;
  second.player.present(screen.view(1), screen.presentation());
  second.open();
  CHECK(first.takeCalls() == Lines{"unload"});
  CHECK(has(second.calls, kSecondLoad));
  first.backend->listener.backendLoaded(media(100));
  first.backend->listener.backendPosition(50);
  CHECK(first.observer->lines == Lines{"status loading"});
  CHECK(first.observer->position == doctest::Approx(42));

  second.player.release();
  CHECK(first.takeCalls() == Lines{"load https://example.com/a.mp4 max 0x0 prebuffer 0"});
  first.backend->listener.backendLoaded(media(100));
  CHECK(first.takeCalls() == Lines{"seek 42.0"});
  first.backend->listener.backendSeekDone();
  CHECK(first.takeCalls() == Lines{"play"});
  CHECK(first.observer->lines == Lines{"status loading", "loaded 100.0", "status readyToPlay"});
}

TEST_CASE("among players no view shows the last to load has the pipeline, and a holder that fails or empties passes it on") {
  Fixture first;
  first.openReady();
  Other second;
  second.open();
  CHECK(first.takeCalls() == Lines{"unload"});
  CHECK(has(second.calls, kSecondLoad));

  second.calls.clear();
  second.backend->listener.backendFailed("The TV's player failed");
  CHECK(second.calls == Lines{"unload"});
  CHECK(first.takeCalls() == Lines{"load https://example.com/a.mp4 max 0x0 prebuffer 0"});

  second.calls.clear();
  second.open();
  CHECK(first.takeCalls() == Lines{"unload"});
  second.player.open("", {});
  CHECK(first.takeCalls() == Lines{"load https://example.com/a.mp4 max 0x0 prebuffer 0"});
}

TEST_CASE("a holder that opens another source keeps the pipeline, and one that waits loads its newest source") {
  Fixture first;
  Screen screen;
  first.openReady();
  first.player.present(screen.view(0), screen.presentation());
  first.calls.clear();
  Other second;
  second.open();
  second.player.open("https://example.com/c.mp4", {});
  second.player.prepare();
  CHECK(second.calls == Lines{"foreground"});

  first.player.open("https://example.com/d.mp4", {});
  first.player.prepare();
  CHECK(first.takeCalls() == Lines{"show 0,0 0x0 at 0,0 1920x1080", "load https://example.com/d.mp4 max 0x0 prebuffer 0"});
  CHECK(second.calls == Lines{"foreground"});

  first.player.release();
  CHECK(second.calls == Lines{"foreground", "load https://example.com/c.mp4 max 0x0 prebuffer 0"});
}

TEST_CASE("a player between two sources does not load when the pipeline comes, and drops its old media when it goes") {
  Fixture first;
  Screen screen;
  first.openReady();
  first.player.present(screen.view(0), screen.presentation());
  Other second;
  second.open();
  second.player.open("https://example.com/c.mp4", {});
  first.player.release();
  CHECK(second.calls == Lines{"foreground"});
  second.player.prepare();
  CHECK(second.calls.back() == "load https://example.com/c.mp4 max 0x0 prebuffer 0");

  second.backend->listener.backendLoaded(media(60));
  second.player.open("https://example.com/d.mp4", {});
  second.calls.clear();
  Other third;
  third.open();
  CHECK(second.calls == Lines{"unload"});
  CHECK(has(third.calls, kSecondLoad));
  CHECK(second.observer->lines == Lines{"status loading", "loaded 60.0", "status readyToPlay"});
}

} // namespace rnwebos::expo
