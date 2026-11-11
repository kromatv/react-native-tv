#pragma once

#include "PlayerModel.h"

#include <folly/dynamic.h>

#include <optional>
#include <vector>

namespace rntizen::expo {

/// expo-video's `AudioTrack` or `SubtitleTrack` for `track`; its id is `<kind>-<index>`.
folly::dynamic trackPayload(TrackKind kind, const MediaTrack &track, int current);

folly::dynamic tracksPayload(TrackKind kind, const std::vector<MediaTrack> &tracks, int current);

/// The selected track of `tracks`, or null.
folly::dynamic currentTrackPayload(TrackKind kind, const std::vector<MediaTrack> &tracks, int current);

/// expo-video's `VideoTrack`, or null before the size is known.
folly::dynamic videoTrackPayload(const VideoStream &video);

/// The index of the track JavaScript passed, by id, then by language; nullopt for null or no match.
std::optional<int> trackIndexOf(TrackKind kind, const folly::dynamic &track, const std::vector<MediaTrack> &tracks);

} // namespace rntizen::expo
