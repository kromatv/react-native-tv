#pragma once

#include "VideoPlayer.h"

#include "../../ExpoTestRuntime.test.h"
#include "../../SharedObjectRegistry.h"

#include <memory>
#include <string>

namespace rntizen::expo::testing {

/// Creates `globalThis.player` with `new VideoPlayer(<source>)`, records every
/// event it emits in `globalThis.events` as `[name, payload]`, and returns its
/// native half.
inline std::shared_ptr<VideoPlayer> createVideoPlayer(ExpoTestRuntime &expo, const std::string &source) {
  expo.eval(
      "globalThis.events = [];"
      "globalThis.player = new expo.modules.ExpoVideo.VideoPlayer(" + source + ");"
      "['statusChange', 'playingChange', 'timeUpdate', 'playToEnd', 'sourceLoad', 'sourceChange',"
      " 'mutedChange', 'volumeChange', 'playbackRateChange', 'availableAudioTracksChange',"
      " 'availableSubtitleTracksChange', 'audioTrackChange', 'subtitleTrackChange', 'videoTrackChange']"
      ".forEach(name => player.addListener(name, (...payload) => events.push([name, ...payload])));");
  const auto id = static_cast<SharedObjectId>(expo.eval("player.__expo_shared_object_id__").getNumber());
  return SharedObjectRegistry::shared().find<VideoPlayer>(id);
}

/// How many `name` events JavaScript has received.
inline int eventCount(ExpoTestRuntime &expo, const std::string &name) {
  return static_cast<int>(expo.eval("events.filter(([n]) => n === '" + name + "').length").getNumber());
}

/// Pumps until JavaScript has received `count` `name` events in all.
inline bool awaitEvents(ExpoTestRuntime &expo, const std::string &name, int count = 1) {
  return expo.pumpUntil([&] { return eventCount(expo, name) >= count; });
}

/// `JSON.stringify` of what `source` evaluates to, with every object's keys
/// sorted: native objects come from folly::dynamic, whose key order is unspecified.
inline std::string canonical(ExpoTestRuntime &expo, const std::string &source) {
  return expo
      .eval(
          "JSON.stringify(" + source +
          ", (key, value) => value && typeof value === 'object' && !Array.isArray(value)"
          " ? Object.fromEntries(Object.keys(value).sort().map(name => [name, value[name]])) : value)")
      .asString(expo.runtime())
      .utf8(expo.runtime());
}

/// The canonical JSON of the payload of the last `name` event, `[]` when it had none.
inline std::string lastEvent(ExpoTestRuntime &expo, const std::string &name) {
  return canonical(expo, "events.filter(([n]) => n === '" + name + "').at(-1).slice(1)");
}

/// The message the simulator's player shim leads every source to.
inline constexpr const char *kNoPlayer =
    "This device has no media player: player_create returned PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE";

} // namespace rntizen::expo::testing
