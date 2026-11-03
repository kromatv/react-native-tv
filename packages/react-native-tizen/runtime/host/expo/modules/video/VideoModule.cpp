#include "VideoPlayer.h"

#include "../../ExpoModuleRegistry.h"
#include "../../ModuleDefinition.h"

namespace rntizen::expo {

namespace {

auto stored(const char *name, folly::dynamic fallback) {
  return [name = std::string(name), fallback = std::move(fallback)](VideoPlayer &player) {
    return player.stored(name, fallback);
  };
}

auto store(const char *name) {
  return [name = std::string(name)](VideoPlayer &player, folly::dynamic value) { player.store(name, value); };
}

ClassDefinition<VideoPlayer> videoPlayerClass(const std::weak_ptr<Context> &context) {
  return ClassDefinition<VideoPlayer>("VideoPlayer")
      .constructor([context](folly::dynamic source, std::optional<bool>, folly::dynamic) {
        auto player = std::make_shared<VideoPlayer>(context);
        player->open(source);
        return player;
      })
      .property("playing", [](VideoPlayer &player) { return player.playing(); })
      .property("status", [](VideoPlayer &player) { return std::string(statusName(player.status())); })
      .property(
          "currentTime",
          [](VideoPlayer &player) { return player.currentTime(); },
          [](VideoPlayer &player, double seconds) { player.setCurrentTime(seconds); })
      .property("duration", [](VideoPlayer &player) { return player.duration(); })
      .property("bufferedPosition", [](VideoPlayer &player) { return player.bufferedPosition(); })
      .property(
          "muted",
          [](VideoPlayer &player) { return player.muted(); },
          [](VideoPlayer &player, bool muted) { player.setMuted(muted); })
      .property(
          "volume",
          [](VideoPlayer &player) { return player.volume(); },
          [](VideoPlayer &player, double volume) { player.setVolume(volume); })
      .property(
          "loop",
          [](VideoPlayer &player) { return player.loop(); },
          [](VideoPlayer &player, bool loop) { player.setLoop(loop); })
      .property(
          "playbackRate",
          [](VideoPlayer &player) { return player.playbackRate(); },
          [](VideoPlayer &player, double rate) { player.setPlaybackRate(rate); })
      .property(
          "timeUpdateEventInterval",
          [](VideoPlayer &player) { return player.timeUpdateEventInterval(); },
          [](VideoPlayer &player, double seconds) { player.setTimeUpdateEventInterval(seconds); })
      .property(
          "bufferOptions",
          [](VideoPlayer &player) { return player.bufferOptions(); },
          [](VideoPlayer &player, folly::dynamic options) { player.setBufferOptions(options); })
      .property(
          "seekTolerance",
          [](VideoPlayer &player) { return player.seekTolerance(); },
          [](VideoPlayer &player, folly::dynamic tolerance) { player.setSeekTolerance(tolerance); })
      .property(
          "maxResolution",
          [](VideoPlayer &player) { return player.maxResolution(); },
          [](VideoPlayer &player, folly::dynamic size) { player.setMaxResolution(size); })
      .property(
          "audioTrack",
          [](VideoPlayer &player) { return player.audioTrack(); },
          [](VideoPlayer &player, folly::dynamic track) { player.setAudioTrack(track); })
      .property(
          "subtitleTrack",
          [](VideoPlayer &player) { return player.subtitleTrack(); },
          [](VideoPlayer &player, folly::dynamic track) { player.setSubtitleTrack(track); })
      .property("availableAudioTracks", [](VideoPlayer &player) { return player.availableAudioTracks(); })
      .property("availableSubtitleTracks", [](VideoPlayer &player) { return player.availableSubtitleTracks(); })
      .property("videoTrack", [](VideoPlayer &player) { return player.videoTrack(); })
      .property("availableVideoTracks", [](VideoPlayer &player) { return player.availableVideoTracks(); })
      .property("isLive", [](VideoPlayer &) { return false; })
      .property("currentLiveTimestamp", [](VideoPlayer &) { return std::optional<double>(); })
      .property("currentOffsetFromLive", [](VideoPlayer &) { return std::optional<double>(); })
      .property("isExternalPlaybackActive", [](VideoPlayer &) { return false; })
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
      .function("replace", [](VideoPlayer &player, folly::dynamic source) { player.replace(source); })
      .asyncFunction("replaceAsync", [](VideoPlayer &player, folly::dynamic source) { player.replace(source); })
      .function("seekBy", [](VideoPlayer &player, double seconds) { player.seekBy(seconds); })
      .function("replay", [](VideoPlayer &player) { player.replay(); })
      .asyncFunction("generateThumbnailsAsync", [](VideoPlayer &, folly::dynamic, folly::dynamic) -> folly::dynamic {
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
