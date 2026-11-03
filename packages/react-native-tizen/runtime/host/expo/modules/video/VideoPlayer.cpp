#include "VideoPlayer.h"

#include "threading/MainThread.h"

#include <algorithm>
#include <atomic>
#include <utility>

namespace rntizen::expo {

namespace {

std::atomic<int> nextNumber{1};

std::string uriOf(const folly::dynamic &source) {
  const auto *uri = source.isObject() ? source.get_ptr("uri") : nullptr;
  if (uri != nullptr && uri->isString()) return uri->getString();
  return source.isString() ? source.getString() : std::string();
}

std::map<std::string, std::string> headersOf(const folly::dynamic &source) {
  std::map<std::string, std::string> headers;
  const auto *field = source.isObject() ? source.get_ptr("headers") : nullptr;
  if (field == nullptr || !field->isObject()) return headers;
  for (const auto &[name, value] : field->items()) {
    if (name.isString() && value.isString()) headers.emplace(name.getString(), value.getString());
  }
  return headers;
}

double numberField(const folly::dynamic &object, const char *name, double fallback) {
  const auto *field = object.isObject() ? object.get_ptr(name) : nullptr;
  return field != nullptr && field->isNumber() ? field->asDouble() : fallback;
}

} // namespace

VideoPlayer::VideoPlayer(std::weak_ptr<Context> context) : context_(std::move(context)), number_(nextNumber++) {}

VideoPlayer::~VideoPlayer() {
  if (player_) runOnMainThread([player = std::move(player_)] { player->release(); });
}

void VideoPlayer::open(const folly::dynamic &source) {
  auto self = std::static_pointer_cast<VideoPlayer>(shared_from_this());
  player_ = std::make_shared<TizenPlayer>(std::weak_ptr<PlayerObserver>(self), number_);
  addMediaSession(self);
  openOnPlayer(source, false);
  if (auto context = context_.lock()) {
    context->runOnJs([weak = std::weak_ptr<VideoPlayer>(self)](facebook::jsi::Runtime &) {
      if (auto player = weak.lock()) player->command([](TizenPlayer &tizen) { tizen.prepare(); });
    });
  }
}

void VideoPlayer::openOnPlayer(const folly::dynamic &source, bool prepareNow) {
  const auto uri = uriOf(source);
  {
    std::lock_guard lock(mutex_);
    snapshot_.source = source;
    snapshot_.status = uri.empty() ? PlayerStatus::Idle : PlayerStatus::Loading;
    snapshot_.position = 0;
    snapshot_.duration = 0;
    snapshot_.buffered = -1;
    snapshot_.media = {};
  }
  command([uri, headers = headersOf(source), prepareNow](TizenPlayer &tizen) {
    tizen.open(uri, headers);
    if (prepareNow) tizen.prepare();
  });
}

void VideoPlayer::replace(const folly::dynamic &source) {
  folly::dynamic old;
  {
    std::lock_guard lock(mutex_);
    old = snapshot_.source;
  }
  openOnPlayer(source, true);
  emit("sourceChange", folly::dynamic::object("source", source)("oldSource", old));
}

void VideoPlayer::play() {
  command([](TizenPlayer &tizen) { tizen.play(); });
}

void VideoPlayer::pause() {
  command([](TizenPlayer &tizen) { tizen.pause(); });
}

void VideoPlayer::seekBy(double seconds) {
  setCurrentTime(currentTime() + seconds);
}

void VideoPlayer::replay() {
  setCurrentTime(0);
  play();
}

bool VideoPlayer::playing() const {
  std::lock_guard lock(mutex_);
  return snapshot_.playing;
}

PlayerStatus VideoPlayer::status() const {
  std::lock_guard lock(mutex_);
  return snapshot_.status;
}

double VideoPlayer::positionLocked() const {
  auto position = snapshot_.position;
  if (snapshot_.playing) {
    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - snapshot_.positionAt;
    position += elapsed.count() * snapshot_.rate;
  }
  return snapshot_.duration > 0 ? std::min(position, snapshot_.duration) : position;
}

double VideoPlayer::currentTime() const {
  std::lock_guard lock(mutex_);
  return positionLocked();
}

void VideoPlayer::setCurrentTime(double seconds) {
  unsigned generation = 0;
  {
    std::lock_guard lock(mutex_);
    snapshot_.position = std::max(0.0, seconds);
    snapshot_.positionAt = std::chrono::steady_clock::now();
    generation = ++snapshot_.seekGeneration;
  }
  command([seconds, generation, accurate = accurateSeeks()](TizenPlayer &tizen) {
    tizen.seekTo(seconds, accurate, generation);
  });
}

double VideoPlayer::duration() const {
  std::lock_guard lock(mutex_);
  return snapshot_.duration;
}

double VideoPlayer::bufferedPosition() const {
  std::lock_guard lock(mutex_);
  return snapshot_.buffered;
}

bool VideoPlayer::muted() const {
  std::lock_guard lock(mutex_);
  return snapshot_.muted;
}

void VideoPlayer::setMuted(bool muted) {
  bool old = false;
  {
    std::lock_guard lock(mutex_);
    old = std::exchange(snapshot_.muted, muted);
  }
  command([muted](TizenPlayer &tizen) { tizen.setMuted(muted); });
  if (old != muted) emit("mutedChange", folly::dynamic::object("muted", muted)("oldMuted", old));
}

double VideoPlayer::volume() const {
  std::lock_guard lock(mutex_);
  return snapshot_.volume;
}

void VideoPlayer::setVolume(double volume) {
  volume = std::clamp(volume, 0.0, 1.0);
  double old = 0;
  {
    std::lock_guard lock(mutex_);
    old = std::exchange(snapshot_.volume, volume);
  }
  command([volume](TizenPlayer &tizen) { tizen.setVolume(volume); });
  if (old != volume) emit("volumeChange", folly::dynamic::object("volume", volume)("oldVolume", old));
}

bool VideoPlayer::loop() const {
  std::lock_guard lock(mutex_);
  return snapshot_.loop;
}

void VideoPlayer::setLoop(bool loop) {
  {
    std::lock_guard lock(mutex_);
    snapshot_.loop = loop;
  }
  command([loop](TizenPlayer &tizen) { tizen.setLooping(loop); });
}

double VideoPlayer::playbackRate() const {
  std::lock_guard lock(mutex_);
  return snapshot_.rate;
}

void VideoPlayer::setPlaybackRate(double rate) {
  double old = 1;
  {
    std::lock_guard lock(mutex_);
    snapshot_.position = positionLocked();
    snapshot_.positionAt = std::chrono::steady_clock::now();
    old = std::exchange(snapshot_.rate, rate);
  }
  command([rate](TizenPlayer &tizen) { tizen.setPlaybackRate(rate); });
  if (old != rate) emit("playbackRateChange", folly::dynamic::object("playbackRate", rate)("oldPlaybackRate", old));
}

double VideoPlayer::timeUpdateEventInterval() const {
  std::lock_guard lock(mutex_);
  return snapshot_.interval;
}

void VideoPlayer::setTimeUpdateEventInterval(double seconds) {
  {
    std::lock_guard lock(mutex_);
    snapshot_.interval = std::max(0.0, seconds);
  }
  command([seconds](TizenPlayer &tizen) { tizen.setProgressInterval(seconds); });
}

folly::dynamic VideoPlayer::bufferOptions() const {
  std::lock_guard lock(mutex_);
  return snapshot_.bufferOptions;
}

void VideoPlayer::setBufferOptions(const folly::dynamic &options) {
  {
    std::lock_guard lock(mutex_);
    snapshot_.bufferOptions = options.isObject() ? options : folly::dynamic::object();
  }
  const auto seconds = numberField(options, "minBufferForPlayback", 0);
  command([seconds](TizenPlayer &tizen) { tizen.setPrebuffer(seconds); });
}

folly::dynamic VideoPlayer::seekTolerance() const {
  std::lock_guard lock(mutex_);
  return snapshot_.seekTolerance;
}

void VideoPlayer::setSeekTolerance(const folly::dynamic &tolerance) {
  std::lock_guard lock(mutex_);
  snapshot_.seekTolerance = tolerance.isObject() ? tolerance : folly::dynamic::object();
}

bool VideoPlayer::accurateSeeks() const {
  std::lock_guard lock(mutex_);
  const auto &tolerance = snapshot_.seekTolerance;
  return numberField(tolerance, "toleranceBefore", -1) == 0 && numberField(tolerance, "toleranceAfter", -1) == 0;
}

folly::dynamic VideoPlayer::maxResolution() const {
  std::lock_guard lock(mutex_);
  return snapshot_.maxResolution;
}

void VideoPlayer::setMaxResolution(const folly::dynamic &size) {
  {
    std::lock_guard lock(mutex_);
    snapshot_.maxResolution = size;
  }
  const int width = static_cast<int>(numberField(size, "width", -1));
  const int height = static_cast<int>(numberField(size, "height", -1));
  command([width, height](TizenPlayer &tizen) { tizen.setMaxResolution(width, height); });
}

folly::dynamic VideoPlayer::stored(const std::string &name, const folly::dynamic &fallback) const {
  std::lock_guard lock(mutex_);
  const auto *value = snapshot_.stored.get_ptr(name);
  return value != nullptr ? *value : fallback;
}

void VideoPlayer::store(const std::string &name, const folly::dynamic &value) {
  std::lock_guard lock(mutex_);
  snapshot_.stored[name] = value;
}

const std::shared_ptr<TizenPlayer> &VideoPlayer::tizenPlayer() const {
  return player_;
}

int VideoPlayer::number() const {
  return number_;
}

void VideoPlayer::sharedObjectDidRelease() {
  command([](TizenPlayer &tizen) { tizen.release(); });
}

void VideoPlayer::command(std::function<void(TizenPlayer &)> body) {
  if (!player_) return;
  runOnMainThread([player = player_, body = std::move(body)] { player->perform(body); });
}

} // namespace rntizen::expo
