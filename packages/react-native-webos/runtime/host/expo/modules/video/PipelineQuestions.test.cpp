#include "PipelineQuestions.h"

#include <doctest/doctest.h>
#include <folly/json.h>

namespace rnwebos::expo {

namespace {

std::string sorted(const folly::dynamic &value) {
  folly::json::serialization_opts options;
  options.sort_keys = true;
  return folly::json::serialize(value, options);
}

} // namespace

TEST_CASE("an HLS load names its transport beside the options, with the app and the window in them") {
  const MediaSource source{.uri = "https://example.com/live/master.m3u8", .headers = {}};
  CHECK(sorted(pipeline::loadPayload(source, "com.example.app", "_Window_Id_39", 1)) ==
        R"({"payload":{"mediaTransportType":"HLS","option":{"appId":"com.example.app",)"
        R"("bufferControl":{"userBufferCtrl":false},"useSeekableRanges":true,"windowId":"_Window_Id_39"}},)"
        R"("type":"media","uri":"https://example.com/live/master.m3u8"})");
}

TEST_CASE("a file load sends no transport, and no window id when the plane has none") {
  const MediaSource source{.uri = "https://example.com/a.mp4", .headers = {}};
  CHECK(sorted(pipeline::loadPayload(source, "com.example.app", "", 1)) ==
        R"({"payload":{"option":{"appId":"com.example.app","bufferControl":{"userBufferCtrl":false},)"
        R"("useSeekableRanges":true}},"type":"media","uri":"https://example.com/a.mp4"})");
}

TEST_CASE("only User-Agent, Cookie and Referer reach the pipeline, and a resolution cap limits the stream") {
  const MediaSource source{
      .uri = "https://example.com/a.mp4",
      .headers = {{"User-Agent", "TV"}, {"cookie", "a=b"}, {"Referer", "https://example.com/"}, {"Authorization", "x"}},
      .prebuffer = 4,
      .maxWidth = 1280,
      .maxHeight = 720,
  };
  const auto option = pipeline::loadPayload(source, "app", "_Window_Id_1", 1)["payload"]["option"];
  CHECK(sorted(option["transmission"]) ==
        R"({"httpHeader":{"cookies":"a=b","referer":"https://example.com/","userAgent":"TV"}})");
  CHECK(sorted(option["adaptiveStreaming"]) == R"({"maxHeight":720,"maxWidth":1280})");
}

TEST_CASE("the calls on a loaded media carry what the pipeline reads") {
  CHECK(sorted(pipeline::subscribePayload("_xyz")) == R"({"mediaId":"_xyz","subscribe":true})");
  CHECK(sorted(pipeline::playRatePayload("_xyz", 1.5)) == R"({"audioOutput":true,"mediaId":"_xyz","playRate":1.5})");
  CHECK(sorted(pipeline::volumePayload("_xyz", 0.25)) ==
        R"({"ease":{"duration":0,"type":"Linear"},"mediaId":"_xyz","volume":25})");
  CHECK(pipeline::volumePayload("_xyz", 7)["volume"] == 100);
}

TEST_CASE("the bus name is a media client name that carries the app and the process") {
  CHECK(pipeline::busNames("com.example.app", 4242) ==
        std::vector<std::string>{"com.webos.media.client.com.example.app.4242"});
  CHECK(pipeline::busNames("", 7) == std::vector<std::string>{"com.webos.media.client.app.7"});
}

TEST_CASE("a refused command or a failed seek leaves the media playing, a missing media does not") {
  for (const int code : {101, 210, 700, 701, 61441, 61442}) CHECK_FALSE(pipeline::endsPlayback(code));
  for (const int code : {100, 104, 200, 201, 202, 300, 601, 1000, 40403}) CHECK(pipeline::endsPlayback(code));
  CHECK(pipeline::bufferedSeconds(17) == 17);
}

} // namespace rnwebos::expo
