#pragma once

#include "PlayerBackend.h"
#include "PlayerTimer.h"
#include "SimulatedPlane.h"
#include "SimulatedStream.h"

#include <chrono>
#include <functional>
#include <memory>
#include <string>

namespace rnwebos::expo {

/// The pipeline of the simulator and of the tests: no network and no
/// decoding, a clock that plays the stream SimulatedStream describes for a
/// URI, reporting what the TV's pipeline reports at about its pace (the
/// position every 200 ms, buffering around a load and a seek, the end of the
/// stream, an error where the stream asks for one). Its plane is a
/// SimulatedPlane. Main thread only.
class SimulatedPlayer final : public PlayerBackend {
 public:
  SimulatedPlayer(PlayerBackendListener &listener, int number);
  ~SimulatedPlayer() override;

  void load(const MediaSource &source) override;
  void unload() override;
  void play() override;
  void pause() override;
  void seek(double seconds) override;
  void setRate(double rate) override;
  void setVolume(double volume) override;
  void selectTrack(TrackKind kind, int index) override;
  void setForeground(bool foreground) override;
  VideoPlane &plane() override;

 private:
  using Clock = std::chrono::steady_clock;

  void after(std::chrono::milliseconds delay, std::function<void()> task);
  void advance();
  void startClock();
  void tick();
  void describe();

  PlayerBackendListener &listener_;
  const int number_;
  const std::shared_ptr<int> alive_{std::make_shared<int>(0)};
  SimulatedPlane plane_;
  PlayerTimer clock_;
  unsigned generation_{0};
  std::string uri_;
  SimulatedStream stream_;
  bool loaded_{false};
  bool playing_{false};
  bool seeking_{false};
  bool foreground_{true};
  double position_{0};
  Clock::time_point positionAt_{};
  double rate_{1};
  double volume_{1};
  int audioTrack_{-1};
  int textTrack_{-1};
};

} // namespace rnwebos::expo
