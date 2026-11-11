#include "SimulatedStream.h"

#include <doctest/doctest.h>

namespace rnwebos::expo {

TEST_CASE("every URI plays as a minute of 1080p with two audio tracks and one text track") {
  const auto stream = simulatedStreamFor("https://example.com/a.mp4");
  CHECK(stream.media.duration == 60);
  CHECK(stream.media.video == VideoStream{.width = 1920, .height = 1080, .fps = 30, .bitrate = 5000000, .codec = "h264"});
  REQUIRE(stream.media.audioTracks.size() == 2);
  CHECK(stream.media.audioTracks[1].language == "fr");
  CHECK(stream.media.audioTrack == 0);
  REQUIRE(stream.media.textTracks.size() == 1);
  CHECK(stream.media.textTrack == -1);
  CHECK(stream.loadError == 0);
  CHECK(stream.errorAt < 0);
}

TEST_CASE("a sim fragment sets the duration, the size, the tracks, the load time and the errors") {
  const auto stream =
      simulatedStreamFor("https://example.com/a.mp4#sim:duration=12.5&size=1280x720&audio=de&text=en+fr+&load=0&error-at=5");
  CHECK(stream.media.duration == 12.5);
  CHECK(stream.media.video.width == 1280);
  CHECK(stream.media.video.height == 720);
  REQUIRE(stream.media.audioTracks.size() == 1);
  CHECK(stream.media.audioTracks[0].language == "de");
  REQUIRE(stream.media.textTracks.size() == 2);
  CHECK(stream.media.textTracks[1].index == 1);
  CHECK(stream.media.textTracks[1].language == "fr");
  CHECK(stream.loadDelay == 0);
  CHECK(stream.errorAt == 5);
  CHECK(simulatedStreamFor("x#sim:error=202").loadError == 202);
}

TEST_CASE("a stream without audio has no current audio track, and a value that is no number changes nothing") {
  const auto silent = simulatedStreamFor("x#sim:audio=&text=");
  CHECK(silent.media.audioTracks.empty());
  CHECK(silent.media.audioTrack == -1);
  CHECK(silent.media.textTracks.empty());

  const auto odd = simulatedStreamFor("x#sim:duration=long&size=wide&flag&=3&size=12xtall");
  CHECK(odd.media.duration == 60);
  CHECK(odd.media.video.width == 12);
  CHECK(odd.media.video.height == 1080);
}

} // namespace rnwebos::expo
