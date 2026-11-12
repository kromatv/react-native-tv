#include "VideoPlayer.h"

#include "../../ExpoModuleRegistry.h"
#include "../../ModuleDefinition.h"

namespace rntizen::expo {

namespace {

auto stored(const char *name, folly::dynamic fallback) {
  return [name = std::string(name), fallback = std::move(fallback)](const VideoPlayer &player) {
    return player.stored(name, fallback);
  };
}

auto store(const char *name) {
  return [name = std::string(name)](VideoPlayer &player, const folly::dynamic &value) { player.store(name, value); };
}

ClassDefinition<VideoPlayer> videoPlayerClass(const std::weak_ptr<Context> &context) {
  return ClassDefinition<VideoPlayer>("VideoPlayer")
      .constructor([context](const folly::dynamic &source, std::optional<bool>, const folly::dynamic &) {
        auto player = std::make_shared<VideoPlayer>(context);
        player->open(source);
        return player;
      })
      .property("playing", [](const VideoPlayer &player) { return player.playing(); })
      .property("status", [](const VideoPlayer &player) { return std::string(statusName(player.status())); })
      .property(
          "currentTime",
          [](const VideoPlayer &player) { return player.currentTime(); },
          [](VideoPlayer &player, double seconds) { player.setCurrentTime(seconds); })
      .property("duration", [](const VideoPlayer &player) { return player.duration(); })
      .property("bufferedPosition", [](const VideoPlayer &player) { return player.bufferedPosition(); })
      .property(
          "muted",
          [](const VideoPlayer &player) { return player.muted(); },
          [](VideoPlayer &player, bool muted) { player.setMuted(muted); })
      .property(
          "volume",
          [](const VideoPlayer &player) { return player.volume(); },
          [](VideoPlayer &player, double volume) { player.setVolume(volume); })
      .property(
          "loop",
          [](const VideoPlayer &player) { return player.loop(); },
          [](VideoPlayer &player, bool loop) { player.setLoop(loop); })
      .property(
          "playbackRate",
          [](const VideoPlayer &player) { return player.playbackRate(); },
          [](VideoPlayer &player, double rate) { player.setPlaybackRate(rate); })
      .property(
          "timeUpdateEventInterval",
          [](const VideoPlayer &player) { return player.timeUpdateEventInterval(); },
          [](VideoPlayer &player, double seconds) { player.setTimeUpdateEventInterval(seconds); })
      .property(
          "bufferOptions",
          [](const VideoPlayer &player) { return player.bufferOptions(); },
          [](VideoPlayer &player, const folly::dynamic &options) { player.setBufferOptions(options); })
      .property(
          "seekTolerance",
          [](const VideoPlayer &player) { return player.seekTolerance(); },
          [](VideoPlayer &player, const folly::dynamic &tolerance) { player.setSeekTolerance(tolerance); })
      .property(
          "maxResolution",
          [](const VideoPlayer &player) { return player.maxResolution(); },
          [](VideoPlayer &player, const folly::dynamic &size) { player.setMaxResolution(size); })
      .property(
          "audioTrack",
          [](const VideoPlayer &player) { return player.audioTrack(); },
          [](VideoPlayer &player, const folly::dynamic &track) { player.setAudioTrack(track); })
      .property(
          "subtitleTrack",
          [](const VideoPlayer &player) { return player.subtitleTrack(); },
          [](VideoPlayer &player, const folly::dynamic &track) { player.setSubtitleTrack(track); })
      .property("availableAudioTracks", [](const VideoPlayer &player) { return player.availableAudioTracks(); })
      .property("availableSubtitleTracks", [](const VideoPlayer &player) { return player.availableSubtitleTracks(); })
      .property("videoTrack", [](const VideoPlayer &player) { return player.videoTrack(); })
      .property("availableVideoTracks", [](const VideoPlayer &player) { return player.availableVideoTracks(); })
      .property("isLive", [](const VideoPlayer &) { return false; })
      .property("currentLiveTimestamp", [](const VideoPlayer &) { return std::optional<double>(); })
      .property("currentOffsetFromLive", [](const VideoPlayer &) { return std::optional<double>(); })
      .property("isExternalPlaybackActive", [](const VideoPlayer &) { return false; })
      .property("targetOffsetFromLive", stored("targetOffsetFromLive", 0), store("targetOffsetFromLive"))
      .property("preservesPitch", stored("preservesPitch", true), store("preservesPitch"))
      .property("keepScreenOnWhilePlaying", stored("keepScreenOnWhilePlaying", true), store("keepScreenOnWhilePlaying"))
      .property("showNowPlayingNotification", stored("showNowPlayingNotification", false), store("showNowPlayingNotification"))
      .property("staysActiveInBackground", stored("staysActiveInBackground", false), store("staysActiveInBackground"))
      .property("allowsExternalPlayback", stored("allowsExternalPlayback", false), store("allowsExternalPlayback"))
      .property("audioMixingMode", stored("audioMixingMode", "auto"), store("audioMixingMode"))
      .property("scrubbingModeOptions", stored("scrubbingModeOptions", folly::dynamic::object()), store("scrubbingModeOptions"))
      .function("play", [](VideoPlayer &player) { player.play(); })
      .function("pause", [](VideoPlayer &player) { player.pause(); })
      .function("replace", [](VideoPlayer &player, const folly::dynamic &source) { player.replace(source); })
      .asyncFunction("replaceAsync", [](VideoPlayer &player, const folly::dynamic &source) { player.replace(source); })
      .function("seekBy", [](VideoPlayer &player, double seconds) { player.seekBy(seconds); })
      .function("replay", [](VideoPlayer &player) { player.replay(); })
      .asyncFunction(
          "generateThumbnailsAsync",
          [](const VideoPlayer &, const folly::dynamic &, const folly::dynamic &) -> folly::dynamic {
            throw CodedError("ERR_UNSUPPORTED", "generateThumbnailsAsync is not supported on Tizen");
          });
}

class ExpoVideoModule final : public Module {
 public:
  ModuleDefinition definition() override {
    return ModuleDefinition("ExpoVideo")
        .function("isPictureInPictureSupported", [] { return false; })
        .function("getCurrentVideoCacheSize", [] { return 0; })
        .asyncFunction("setVideoCacheSizeAsync", [](double) {})
        .asyncFunction("clearVideoCacheAsync", [] {})
        .sharedClass(videoPlayerClass(context().weak_from_this()));
  }
};

} // namespace

RN_TIZEN_EXPO_MODULE(ExpoVideo, ExpoVideoModule);

} // namespace rntizen::expo
