#pragma once

#include "LunaMediaEvents.h"
#include "LunaMediaUnload.h"
#include "PlayerBackend.h"
#include "PlayerTimer.h"

#include "platform/LunaConnection.h"

#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>

namespace rnwebos::expo {

/// The TV's own media pipeline, driven through `luna://com.webos.media` in
/// the call sequence docs/reference/webos-media-playback.md lays out: `load`
/// with the plane's window id, `notifyForeground`, `subscribe`, then the
/// events of that subscription. The pipeline fetches, demuxes, decodes and
/// draws subtitles itself. Main thread only.
class LunaMediaPlayer final : public PlayerBackend {
 public:
  /// `bus` must outlive the player: the pipeline unloads a media when the
  /// client that loaded it leaves the bus. Call `unload` before destroying a
  /// player another one follows, so that its load waits for this media to go.
  LunaMediaPlayer(
      LunaConnection &bus,
      std::unique_ptr<VideoPlane> plane,
      PlayerBackendListener &listener,
      std::string appId,
      int number);
  ~LunaMediaPlayer() override;

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
  using Answer = std::function<void(bool accepted)>;
  using Track = std::pair<TrackKind, int>;

  void start(const MediaSource &source);
  void send(const std::string &method, const folly::dynamic &payload, Answer onAnswer = {});
  folly::dynamic media() const;
  void later(std::function<void()> task);
  void fail(const std::string &message);
  void requestPlaying(bool playing);

  void onLoadReply(const folly::dynamic &reply, unsigned generation);
  void onEvent(const folly::dynamic &message);
  void onLoadCompleted();
  void onSourceInfo(const LunaMediaEvent &event);
  void onVideoInfo(const VideoStream &video);
  void onError(const LunaMediaEvent &event);
  void onUnloaded();
  void noteFirst(const LunaMediaEvent &event, const folly::dynamic &message);
  VideoStream currentVideo() const;
  void setPlaying(bool playing);
  void finishSeek();
  void confirmTrack(TrackKind kind, int index);

  LunaConnection &bus_;
  const std::shared_ptr<VideoPlane> plane_;
  PlayerBackendListener &listener_;
  const std::string appId_;
  const int number_;
  const std::shared_ptr<int> alive_{std::make_shared<int>(0)};
  unsigned generation_{0};
  std::string mediaId_;
  std::unique_ptr<LunaConnection::Subscription> events_;
  LunaEventSink sink_;
  std::set<std::string, std::less<>> seenEvents_;
  LoadedMedia media_;
  VideoStream sourceVideo_;
  VideoStream liveVideo_;
  bool loadCompleted_{false};
  bool sourceKnown_{false};
  bool foreground_{true};
  bool playing_{false};
  bool seeking_{false};
  int textTrack_{-1};
  std::optional<bool> awaitedPlaying_;
  std::optional<Track> awaitedTrack_;
  PlayerTimer playWait_;
  PlayerTimer seekWait_;
  PlayerTimer trackWait_;
};

} // namespace rnwebos::expo
