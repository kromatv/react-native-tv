#include "SimulatedPlayer.h"

#include "LunaMediaEvents.h"
#include "PlayerLog.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace rnwebos::expo {

namespace {

constexpr std::chrono::milliseconds kTick{200};
constexpr std::chrono::milliseconds kSeekTime{120};
constexpr double kBufferAhead = 10;
constexpr int kPlaybackError = 300;
constexpr std::size_t kMaxCaptionUri = 90;

std::chrono::milliseconds millis(double seconds) {
  return std::chrono::milliseconds(static_cast<std::int64_t>(std::llround(std::max(0.0, seconds) * 1000)));
}

std::string clockTime(double seconds) {
  const int whole = static_cast<int>(seconds);
  return std::format("{}:{:02}.{}", whole / 60, whole % 60, static_cast<int>((seconds - whole) * 10));
}

std::string language(const std::vector<MediaTrack> &tracks, int index) {
  for (const auto &track : tracks) {
    if (track.index == index) return track.language;
  }
  return "off";
}

} // namespace

SimulatedPlayer::SimulatedPlayer(PlayerBackendListener &listener, int number) : listener_(listener), number_(number) {}

SimulatedPlayer::~SimulatedPlayer() = default;

VideoPlane &SimulatedPlayer::plane() {
  return plane_;
}

void SimulatedPlayer::after(std::chrono::milliseconds delay, std::function<void()> task) {
  MainLoop::get().addTimer(
      delay, [this, alive = std::weak_ptr<int>(alive_), generation = generation_, task = std::move(task)] {
        if (!alive.expired() && generation == generation_) task();
      });
}

void SimulatedPlayer::load(const MediaSource &source) {
  unload();
  uri_ = source.uri;
  stream_ = simulatedStreamFor(source.uri);
  audioTrack_ = stream_.media.audioTrack;
  playerLog(number_, std::format("simulated load of {}: {} s", uri_, decimal(stream_.media.duration, 1)));
  describe();
  after(millis(stream_.loadDelay), [this] {
    if (stream_.loadError != 0) {
      listener_.backendFailed("The TV cannot open the source: " + describeMediaError(stream_.loadError, "simulated"));
      return;
    }
    loaded_ = true;
    describe();
    listener_.backendLoaded(stream_.media);
    listener_.backendBuffered(std::min(stream_.media.duration, kBufferAhead));
  });
}

void SimulatedPlayer::unload() {
  ++generation_;
  clock_.cancel();
  loaded_ = false;
  playing_ = false;
  seeking_ = false;
  position_ = 0;
  textTrack_ = -1;
  uri_.clear();
  describe();
}

void SimulatedPlayer::play() {
  if (!loaded_ || playing_) return;
  playing_ = true;
  positionAt_ = Clock::now();
  if (!seeking_) startClock();
  describe();
  after({}, [this] { listener_.backendPlayingChanged(true); });
}

void SimulatedPlayer::pause() {
  if (!loaded_ || !playing_) return;
  advance();
  playing_ = false;
  clock_.cancel();
  describe();
  after({}, [this] { listener_.backendPlayingChanged(false); });
}

void SimulatedPlayer::seek(double seconds) {
  if (!loaded_) return;
  advance();
  seeking_ = true;
  clock_.cancel();
  after({}, [this] { listener_.backendBuffering(true); });
  after(kSeekTime, [this, seconds] {
    seeking_ = false;
    position_ = std::clamp(seconds, 0.0, stream_.media.duration);
    positionAt_ = Clock::now();
    describe();
    listener_.backendSeekDone();
    listener_.backendPosition(position_);
    listener_.backendBuffering(false);
    if (playing_) startClock();
  });
}

void SimulatedPlayer::setRate(double rate) {
  advance();
  rate_ = rate;
  describe();
}

void SimulatedPlayer::setVolume(double volume) {
  volume_ = volume;
  describe();
}

void SimulatedPlayer::selectTrack(TrackKind kind, int index) {
  if (!loaded_) return;
  (kind == TrackKind::Audio ? audioTrack_ : textTrack_) = index;
  describe();
  if (index >= 0) after({}, [this, kind, index] { listener_.backendTrackChanged(kind, index); });
}

void SimulatedPlayer::setForeground(bool foreground) {
  foreground_ = foreground;
  describe();
}

void SimulatedPlayer::advance() {
  const auto now = Clock::now();
  if (playing_ && !seeking_) {
    position_ += std::chrono::duration<double>(now - positionAt_).count() * rate_;
    position_ = std::clamp(position_, 0.0, stream_.media.duration);
  }
  positionAt_ = now;
}

void SimulatedPlayer::startClock() {
  clock_.start(kTick, [this] { tick(); });
}

void SimulatedPlayer::tick() {
  advance();
  describe();
  const auto duration = stream_.media.duration;
  if (stream_.errorAt >= 0 && position_ >= stream_.errorAt) {
    playing_ = false;
    describe();
    listener_.backendFailed("The TV's player failed: " + describeMediaError(kPlaybackError, "simulated"));
    return;
  }
  listener_.backendPosition(position_);
  listener_.backendBuffered(std::min(duration, position_ + kBufferAhead));
  if (position_ >= duration) {
    playing_ = false;
    describe();
    listener_.backendEnded();
    return;
  }
  startClock();
}

void SimulatedPlayer::describe() {
  if (uri_.empty()) {
    plane_.setPicture({}, "No source", {});
    return;
  }
  const auto &media = stream_.media;
  const char *state = !loaded_ ? "loading" : seeking_ ? "seeking" : playing_ ? "playing" : "paused";
  auto caption = std::format(
      "Simulated video {}: {}\n{} of {} · {} · {}x · volume {} %{}\n{}x{} {} · audio {} · text {}",
      number_,
      uri_.substr(0, kMaxCaptionUri),
      clockTime(position_),
      clockTime(media.duration),
      state,
      decimal(rate_, 2),
      static_cast<int>(std::lround(volume_ * 100)),
      foreground_ ? "" : " · in the background",
      media.video.width,
      media.video.height,
      media.video.codec,
      language(media.audioTracks, audioTrack_),
      language(media.textTracks, textTrack_));
  auto subtitle = textTrack_ >= 0 ? std::format("Subtitles ({}), drawn by the pipeline", language(media.textTracks, textTrack_))
                                  : std::string();
  plane_.setPicture({media.video.width, media.video.height}, std::move(caption), std::move(subtitle));
}

} // namespace rnwebos::expo
