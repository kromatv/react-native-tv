#pragma once

#include <chrono>
#include <optional>

namespace rntizen::expo {

/// Tells when the overlay plane shows a picture of the current source.
/// capi-media-player reports no rendered frame for an overlay, so the picture
/// counts as there once playback has moved on from where it ran from, or has
/// run a while with no position to go by (`tizen-media-playback.md`, section 3).
class PictureWatch final {
 public:
  using Clock = std::chrono::steady_clock;

  enum class Arrival { NotYet, Moved, Assumed };

  /// Playback runs from `positionMs`: the picture is due once it moves on.
  void expect(int positionMs, Clock::time_point now);
  /// Playback paused or seeks: nothing is due until the next expect().
  void hold();
  /// The player buffers: a position that stands still is no sign of a picture yet.
  void stalled(Clock::time_point now);
  /// Takes the position read while a picture is due, nullopt when the player
  /// gave none, and tells whether the picture arrived with it.
  Arrival sample(std::optional<int> positionMs, Clock::time_point now);
  /// The source changed, failed or lost its decoder: no picture until playback moves again.
  void lose();

  bool shown() const;
  bool due() const;

 private:
  enum class State { Absent, Due, Shown };

  State state_{State::Absent};
  int fromMs_{0};
  Clock::time_point since_{};
};

} // namespace rntizen::expo
