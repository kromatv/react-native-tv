#include "MediaSessions.h"

#include <doctest/doctest.h>

#include <memory>

namespace rntizen {

namespace {

class FakeSession final : public MediaSession {
 public:
  FakeSession(bool playing, bool staysActive) : playing_(playing), staysActive_(staysActive) {}

  bool isPlaying() const override {
    return playing_;
  }
  bool staysActiveInBackground() const override {
    return staysActive_;
  }
  void play() override {
    playing_ = true;
  }
  void pause() override {
    playing_ = false;
  }
  void restart() override {}
  void seekBy(double) override {}
  void seekTo(double) override {}
  bool setSubtitlesEnabled(bool) override {
    return false;
  }

 private:
  bool playing_;
  bool staysActive_;
};

} // namespace

TEST_CASE("currentMediaSession prefers the one playing, then the one last played, then the newest") {
  const auto older = std::make_shared<FakeSession>(false, false);
  const auto played = std::make_shared<FakeSession>(false, false);
  const auto newest = std::make_shared<FakeSession>(false, false);
  addMediaSession(older);
  addMediaSession(played);
  addMediaSession(newest);

  CHECK(currentMediaSession() == newest);
  markMediaSessionActive(played.get());
  CHECK(currentMediaSession() == played);
  older->play();
  CHECK(currentMediaSession() == older);

  markMediaSessionActive(nullptr);
}

TEST_CASE("currentMediaSession forgets a destroyed session") {
  auto session = std::make_shared<FakeSession>(true, false);
  addMediaSession(session);

  session.reset();

  CHECK(currentMediaSession() == nullptr);
}

TEST_CASE("pauseMediaSessionsForBackground pauses every player that may not play hidden") {
  const auto foreground = std::make_shared<FakeSession>(true, false);
  const auto background = std::make_shared<FakeSession>(true, true);
  const auto idle = std::make_shared<FakeSession>(false, false);
  addMediaSession(foreground);
  addMediaSession(background);
  addMediaSession(idle);

  pauseMediaSessionsForBackground(false);
  pauseMediaSessionsForBackground(true);

  CHECK_FALSE(foreground->isPlaying());
  CHECK(background->isPlaying());
  CHECK_FALSE(idle->isPlaying());
}

TEST_CASE("pauseMediaSessionsForBackground pauses the only player when it may not play hidden") {
  const auto player = std::make_shared<FakeSession>(true, false);
  addMediaSession(player);

  pauseMediaSessionsForBackground(true);

  CHECK_FALSE(player->isPlaying());
}

} // namespace rntizen
