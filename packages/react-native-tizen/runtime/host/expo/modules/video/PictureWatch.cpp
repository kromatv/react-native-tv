#include "PictureWatch.h"

namespace rntizen::expo {

namespace {

// capi-media-player is PLAYING once player_start returns, before the plane has a
// frame: a few frames of its clock come first (tizen-media-playback.md, section 3).
constexpr int kMovedMs = 100;
// A capi source whose position never moves, as a live stream's may, would stay black.
constexpr auto kUnreported = std::chrono::seconds(2);

} // namespace

void PictureWatch::expect(int positionMs, Clock::time_point now) {
  if (state_ == State::Shown) return;
  state_ = State::Due;
  fromMs_ = positionMs;
  since_ = now;
}

void PictureWatch::hold() {
  if (state_ == State::Due) state_ = State::Absent;
}

void PictureWatch::stalled(Clock::time_point now) {
  if (state_ == State::Due) since_ = now;
}

PictureWatch::Arrival PictureWatch::sample(std::optional<int> positionMs, Clock::time_point now) {
  using enum Arrival;
  if (state_ != State::Due) return NotYet;
  if (positionMs && *positionMs < fromMs_) fromMs_ = *positionMs;
  const bool moved = positionMs && *positionMs - fromMs_ >= kMovedMs;
  if (!moved && now - since_ < kUnreported) return NotYet;
  state_ = State::Shown;
  return moved ? Moved : Assumed;
}

void PictureWatch::lose() {
  state_ = State::Absent;
}

bool PictureWatch::shown() const {
  return state_ == State::Shown;
}

bool PictureWatch::due() const {
  return state_ == State::Due;
}

} // namespace rntizen::expo
