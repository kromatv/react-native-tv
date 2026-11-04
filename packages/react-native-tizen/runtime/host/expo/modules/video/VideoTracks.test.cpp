#include "VideoTracks.h"

#include <doctest/doctest.h>

namespace rntizen::expo {

namespace {

const std::vector<MediaTrack> kTracks{{.index = 0, .language = "en"}, {.index = 1, .language = "und"}};

} // namespace

TEST_CASE("trackPayload names a track by its language, or by its position when the file names none") {
  const auto english = trackPayload(TrackKind::Audio, kTracks[0], 0);
  CHECK(english["id"] == "audio-0");
  CHECK(english["language"] == "en");
  CHECK(english["label"] == "en");
  CHECK(english["name"] == "en");
  CHECK(english["isDefault"] == true);
  CHECK(english["autoSelect"] == true);

  const auto unnamed = trackPayload(TrackKind::Text, kTracks[1], 0);
  CHECK(unnamed["id"] == "text-1");
  CHECK(unnamed["language"] == "und");
  CHECK(unnamed["label"] == "Track 2");
  CHECK(unnamed["isDefault"] == false);
  CHECK(unnamed["autoSelect"] == false);
}

TEST_CASE("tracksPayload lists every track and currentTrackPayload the selected one") {
  const auto list = tracksPayload(TrackKind::Audio, kTracks, 1);
  REQUIRE(list.size() == 2);
  CHECK(list[0]["autoSelect"] == false);
  CHECK(list[1]["autoSelect"] == true);
  CHECK(tracksPayload(TrackKind::Audio, {}, 0).empty());

  CHECK(currentTrackPayload(TrackKind::Audio, kTracks, 1)["id"] == "audio-1");
  CHECK(currentTrackPayload(TrackKind::Audio, kTracks, 5).isNull());
  CHECK(currentTrackPayload(TrackKind::Text, {}, -1).isNull());
}

TEST_CASE("videoTrackPayload describes the stream once its size is known") {
  CHECK(videoTrackPayload({}).isNull());
  CHECK(videoTrackPayload({.width = 1920, .height = 0}).isNull());

  const auto full = videoTrackPayload({.width = 1920, .height = 1080, .fps = 25, .bitrate = 4000000, .codec = "h264"});
  CHECK(full["size"]["width"] == 1920);
  CHECK(full["size"]["height"] == 1080);
  CHECK(full["id"] == "video-0");
  CHECK(full["mimeType"] == "h264");
  CHECK(full["frameRate"] == 25);
  CHECK(full["bitrate"] == 4000000);
  CHECK(full["isSupported"] == true);
  CHECK(full["url"].isNull());
  CHECK(full["averageBitrate"].isNull());
  CHECK(full["peakBitrate"].isNull());

  const auto bare = videoTrackPayload({.width = 640, .height = 360});
  CHECK(bare["mimeType"].isNull());
  CHECK(bare["frameRate"].isNull());
  CHECK(bare["bitrate"].isNull());
}

TEST_CASE("trackIndexOf finds a track by id first, then by language, never across kinds") {
  CHECK(trackIndexOf(TrackKind::Audio, folly::dynamic::object("id", "audio-1"), kTracks) == 1);
  CHECK(trackIndexOf(TrackKind::Audio, folly::dynamic::object("language", "en"), kTracks) == 0);
  CHECK_FALSE(trackIndexOf(TrackKind::Audio, folly::dynamic::object("id", "text-1"), kTracks).has_value());
  CHECK_FALSE(trackIndexOf(TrackKind::Audio, folly::dynamic::object("id", "audio-9")("language", "en"), kTracks)
                  .has_value());
  CHECK_FALSE(trackIndexOf(TrackKind::Audio, folly::dynamic::object("language", "de"), kTracks).has_value());
  CHECK_FALSE(trackIndexOf(TrackKind::Audio, folly::dynamic::object("id", 1), kTracks).has_value());
  CHECK_FALSE(trackIndexOf(TrackKind::Audio, nullptr, kTracks).has_value());
  CHECK_FALSE(trackIndexOf(TrackKind::Audio, "audio-0", kTracks).has_value());
}

} // namespace rntizen::expo
