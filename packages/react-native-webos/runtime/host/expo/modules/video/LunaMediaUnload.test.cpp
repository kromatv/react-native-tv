#include "LunaMediaPlayerHarness.test.h"

#include <doctest/doctest.h>

namespace rnwebos::expo {

using testing::FakeLunaBus;
using testing::kLoaded;
using testing::pumpFor;
using testing::pumpUntil;
using testing::RecordingListener;

namespace {

constexpr auto kLate = R"({"returnValue":true,"mediaId":"_late"})";

} // namespace

TEST_CASE("a load waits for the answer to the load before it, whose media is unloaded first") {
  testing::LunaFixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  bus.completesUnloads = false;
  player.load({.uri = "https://example.com/a.mp4", .headers = {}});
  player.load({.uri = "https://example.com/b.mp4", .headers = {}});
  pumpFor(std::chrono::milliseconds(50));
  CHECK(bus.count("load") == 1);
  bus.answer("load", kLate);
  CHECK(bus.payloadOf("unload") == R"({"mediaId":"_late"})");
  CHECK(bus.count("load") == 1);
  CHECK(pumpUntil([&] { return bus.count("load") == 2; }));
  CHECK(bus.payloadOf("load").find("b.mp4") != std::string::npos);
  CHECK(listener.lines.empty());
}

TEST_CASE("another player's load waits for that answer too, and goes ahead when it never comes") {
  rnwebos::testing::MainLoopScope mainLoop;
  FakeLunaBus bus;
  RecordingListener listener;
  LunaMediaPlayer first(bus, std::make_unique<testing::FixedPlane>(), listener, "com.example.app", 1);
  LunaMediaPlayer second(bus, std::make_unique<testing::FixedPlane>(), listener, "com.example.app", 2);
  first.load({.uri = "https://example.com/a.mp4", .headers = {}});
  first.unload();
  second.load({.uri = "https://example.com/b.mp4", .headers = {}});
  CHECK(bus.count("load") == 1);
  CHECK(pumpUntil([&] { return bus.count("load") == 2; }));
  bus.answer("load", kLoaded);
  CHECK(bus.payloadOf("subscribe") == R"({"mediaId":"_xyz","subscribe":true})");
}

TEST_CASE("a player destroyed before its load is answered still unloads that media, and holds no later load") {
  rnwebos::testing::MainLoopScope mainLoop;
  FakeLunaBus bus;
  RecordingListener listener;
  auto first = std::make_unique<LunaMediaPlayer>(bus, std::make_unique<testing::FixedPlane>(), listener, "com.example.app", 1);
  first->load({.uri = "https://example.com/a.mp4", .headers = {}});
  first.reset();
  CHECK(bus.count("unload") == 0);
  bus.answer("load", kLate);
  CHECK(bus.payloadOf("unload") == R"({"mediaId":"_late"})");

  LunaMediaPlayer second(bus, std::make_unique<testing::FixedPlane>(), listener, "com.example.app", 2);
  second.load({.uri = "https://example.com/b.mp4", .headers = {}});
  CHECK(bus.count("load") == 1);
  CHECK(pumpUntil([&] { return bus.count("load") == 2; }));
  bus.answer("load", kLoaded);
}

TEST_CASE("a bus that closes with a load unanswered leaves the next loads free") {
  rnwebos::testing::MainLoopScope mainLoop;
  RecordingListener listener;
  {
    FakeLunaBus closing;
    LunaMediaPlayer player(closing, std::make_unique<testing::FixedPlane>(), listener, "com.example.app", 1);
    player.load({.uri = "https://example.com/a.mp4", .headers = {}});
  }
  FakeLunaBus bus;
  LunaMediaPlayer player(bus, std::make_unique<testing::FixedPlane>(), listener, "com.example.app", 2);
  player.load({.uri = "https://example.com/b.mp4", .headers = {}});
  CHECK(bus.count("load") == 1);
  bus.answer("load", kLoaded);
}

} // namespace rnwebos::expo
