#include "LunaMediaPlayer.h"

#include "PipelineQuestions.h"
#include "PlayerLog.h"

#include "platform/MainLoop.h"

#include <folly/json.h>

#include <cmath>
#include <format>

namespace rnwebos::expo {

namespace {

constexpr const char *kService = "luna://com.webos.media/";

folly::dynamic *httpHeaderOf(folly::dynamic &payload) {
  auto *option = payload["payload"].get_ptr("option");
  auto *transmission = option != nullptr ? option->get_ptr("transmission") : nullptr;
  return transmission != nullptr ? transmission->get_ptr("httpHeader") : nullptr;
}

// A cookie or a signed referer is a credential: the log says only that it was sent.
std::string loggable(folly::dynamic payload) {
  if (auto *header = httpHeaderOf(payload); header != nullptr) {
    for (auto &value : header->values()) value = "(sent)";
  }
  return folly::toJson(payload);
}

} // namespace

LunaMediaPlayer::LunaMediaPlayer(
    LunaConnection &bus,
    std::unique_ptr<VideoPlane> plane,
    PlayerBackendListener &listener,
    std::string appId,
    int number)
    : bus_(bus), plane_(std::move(plane)), listener_(listener), appId_(std::move(appId)), number_(number) {}

// Sent without waiting for it to complete: an owner that unloads in time calls
// unload() first, and the one left is the closing app, whose main loop is gone.
LunaMediaPlayer::~LunaMediaPlayer() {
  if (sink_) *sink_ = nullptr;
  if (!mediaId_.empty()) bus_.call(std::string(kService) + "unload", media());
}

VideoPlane &LunaMediaPlayer::plane() {
  return *plane_;
}

folly::dynamic LunaMediaPlayer::media() const {
  return folly::dynamic::object("mediaId", mediaId_);
}

void LunaMediaPlayer::later(std::function<void()> task) {
  MainLoop::get().post([alive = std::weak_ptr<int>(alive_), task = std::move(task)] {
    if (!alive.expired()) task();
  });
}

void LunaMediaPlayer::fail(const std::string &message) {
  listener_.backendFailed(message);
}

void LunaMediaPlayer::send(const std::string &method, const folly::dynamic &payload, Answer onAnswer) {
  playerLog(number_, std::format("{} {}", method, folly::toJson(payload)), LogLevel::Debug);
  auto onReply = [this, alive = std::weak_ptr<int>(alive_), method, onAnswer = std::move(onAnswer)](
                     const folly::dynamic &reply) {
    if (alive.expired()) return;
    const auto answer = parseLunaMediaEvent(reply);
    if (!answer.accepted) {
      const auto why = describeMediaError(answer.errorCode, answer.errorText);
      playerLog(number_, std::format("{} refused: {}", method, why), LogLevel::Warning);
    }
    if (onAnswer) onAnswer(answer.accepted);
  };
  if (!bus_.call(kService + method, payload, std::move(onReply))) {
    playerLog(number_, "the Luna bus did not take " + method, LogLevel::Warning);
  }
}

void LunaMediaPlayer::load(const MediaSource &source) {
  unload();
  afterUnloads([this, alive = std::weak_ptr<int>(alive_), source, generation = generation_] {
    if (!alive.expired() && generation == generation_) start(source);
  });
}

void LunaMediaPlayer::start(const MediaSource &source) {
  media_ = {};
  sourceVideo_ = {};
  liveVideo_ = {};
  seenEvents_.clear();
  loadCompleted_ = false;
  sourceKnown_ = false;
  playing_ = false;
  textTrack_ = -1;
  const auto payload = pipeline::loadPayload(source, appId_, plane_->windowId(), number_);
  playerLog(number_, "load " + loggable(payload));
  auto onReply = [this, alive = std::weak_ptr<int>(alive_), generation = generation_](const folly::dynamic &reply) {
    if (!alive.expired()) onLoadReply(reply, generation);
  };
  if (!bus_.call(std::string(kService) + "load", payload, std::move(onReply))) {
    later([this, generation = generation_] {
      if (generation == generation_) fail("The TV's media service did not take the load");
    });
  }
}

void LunaMediaPlayer::onLoadReply(const folly::dynamic &reply, unsigned generation) {
  const auto answer = parseLunaMediaEvent(reply);
  const auto *id = reply.isObject() ? reply.get_ptr("mediaId") : nullptr;
  const auto mediaId = id != nullptr && id->isString() ? id->getString() : std::string();
  if (generation != generation_) {
    if (!mediaId.empty()) unloadMedia(bus_, mediaId, nullptr, nullptr, plane_, number_);
    return;
  }
  if (!answer.accepted || mediaId.empty()) {
    fail("The TV cannot open the source: " + describeMediaError(answer.errorCode, answer.errorText));
    return;
  }
  mediaId_ = mediaId;
  playerLog(number_, "loading as " + mediaId_);
  setForeground(foreground_);
  sink_ = std::make_shared<std::function<void(const folly::dynamic &)>>(
      [this, alive = std::weak_ptr<int>(alive_)](const folly::dynamic &message) {
        if (!alive.expired()) onEvent(message);
      });
  events_ = bus_.subscribe(
      std::string(kService) + "subscribe",
      pipeline::subscribePayload(mediaId_),
      [sink = sink_](const folly::dynamic &message) {
        // Copied: handling an event may hand the sink over to an unload.
        if (const auto deliver = *sink) deliver(message);
      });
  if (!events_) fail("The TV's media service did not take the subscription");
}

void LunaMediaPlayer::unload() {
  ++generation_;
  awaited_.playWait.cancel();
  awaited_.seekWait.cancel();
  awaited_.trackWait.cancel();
  awaited_.playing.reset();
  awaited_.track.reset();
  awaited_.seek = false;
  if (mediaId_.empty()) return;
  playerLog(number_, "unload " + mediaId_);
  unloadMedia(bus_, std::exchange(mediaId_, {}), std::move(events_), std::move(sink_), plane_, number_);
}

void LunaMediaPlayer::play() {
  requestPlaying(true);
}

void LunaMediaPlayer::pause() {
  requestPlaying(false);
}

void LunaMediaPlayer::requestPlaying(bool playing) {
  if (mediaId_.empty()) return;
  awaited_.playing = playing;
  send(playing ? "play" : "pause", media(), [this, playing](bool accepted) {
    if (!accepted || awaited_.playing != playing) return;
    awaited_.playWait.start(pipeline::kPlayStateWait, [this, playing] {
      pipeline::openQuestion(number_, std::format("no {} event: assumed", playing ? "playing" : "paused"));
      setPlaying(playing);
    });
  });
}

void LunaMediaPlayer::seek(double seconds) {
  if (mediaId_.empty()) return;
  awaited_.seek = true;
  const auto position = static_cast<std::int64_t>(std::llround(seconds * 1000));
  send("seek", folly::dynamic::object("mediaId", mediaId_)("position", position), [this](bool accepted) {
    if (!awaited_.seek) return;
    if (!accepted) {
      finishSeek();
      return;
    }
    awaited_.seekWait.start(pipeline::kSeekDoneWait, [this] {
      pipeline::openQuestion(number_, "no seekDone after seek: assumed done");
      finishSeek();
    });
  });
}

void LunaMediaPlayer::setRate(double rate) {
  if (mediaId_.empty()) return;
  if (rate != 1) pipeline::openQuestion(number_, "setPlayRate off 1.0: LG documents it as unsupported on streams");
  send("setPlayRate", pipeline::playRatePayload(mediaId_, rate));
}

void LunaMediaPlayer::setVolume(double volume) {
  if (mediaId_.empty()) return;
  send("setVolume", pipeline::volumePayload(mediaId_, volume), [this](bool accepted) {
    if (!accepted) pipeline::openQuestion(number_, "setVolume refused: the player's volume stays the TV's");
  });
}

void LunaMediaPlayer::selectTrack(TrackKind kind, int index) {
  if (mediaId_.empty()) return;
  if (kind == TrackKind::Text && index < 0) {
    textTrack_ = -1;
    send("setSubtitleEnable", folly::dynamic::object("mediaId", mediaId_)("enable", false));
    return;
  }
  const char *type = kind == TrackKind::Audio ? "audio" : "text";
  awaited_.track = Track(kind, index);
  send(
      "selectTrack",
      folly::dynamic::object("mediaId", mediaId_)("type", type)("index", index),
      [this, kind, index](bool accepted) {
        if (!accepted || awaited_.track != Track(kind, index)) return;
        awaited_.trackWait.start(pipeline::kTrackSelectedWait, [this, kind, index] {
          pipeline::openQuestion(number_, "no trackSelected after selectTrack: assumed selected");
          confirmTrack(kind, index);
        });
      });
  if (kind == TrackKind::Text) send("setSubtitleEnable", folly::dynamic::object("mediaId", mediaId_)("enable", true));
}

void LunaMediaPlayer::setForeground(bool foreground) {
  foreground_ = foreground;
  if (mediaId_.empty()) return;
  send(foreground ? "notifyForeground" : "notifyBackground", folly::dynamic::object("connectionId", mediaId_));
}

} // namespace rnwebos::expo
