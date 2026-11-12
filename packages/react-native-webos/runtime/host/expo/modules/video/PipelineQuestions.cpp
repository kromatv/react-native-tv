#include "PipelineQuestions.h"

#include "PlayerLog.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <format>

namespace rnwebos::expo::pipeline {

namespace {

std::string lowered(std::string text) {
  std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return text;
}

// Open question 10: `httpHeader` has three fields and no way to pass another header.
folly::dynamic httpHeader(const SourceHeaders &headers, int player) {
  folly::dynamic header = folly::dynamic::object();
  for (const auto &[name, value] : headers) {
    const auto key = lowered(name);
    if (key == "user-agent") {
      header["userAgent"] = value;
    } else if (key == "cookie") {
      header["cookies"] = value;
    } else if (key == "referer") {
      header["referer"] = value;
    } else {
      playerLog(
          player,
          std::format("header {} dropped: the TV's player takes only User-Agent, Cookie and Referer", name),
          LogLevel::Warning);
    }
  }
  return header;
}

} // namespace

void openQuestion(int player, const std::string &what) {
  playerLog(player, "open question: " + what, LogLevel::Debug);
}

// Open question 1. A fixed name stays held for a while after a killed process,
// so the name carries the process id (nuvio's fallback, the reference's first choice).
std::vector<std::string> busNames(const std::string &appId, int processId) {
  const auto owner = appId.empty() ? std::string("app") : appId;
  return {std::format("com.webos.media.client.{}.{}", owner, processId)};
}

// Open question 2. As LG's plugin: `mediaTransportType` beside `option` for an
// HLS URL, nothing for a file. nuvio puts it inside `option`, plx-native sends none.
folly::dynamic loadPayload(const MediaSource &source, const std::string &appId, const std::string &windowId, int player) {
  folly::dynamic option = folly::dynamic::object("appId", appId)("useSeekableRanges", true)(
      "bufferControl", folly::dynamic::object("userBufferCtrl", false));
  if (windowId.empty()) {
    openQuestion(player, "load without option.windowId: webOS 5 and later answer error 104");
  } else {
    option["windowId"] = windowId;
  }
  if (auto header = httpHeader(source.headers, player); !header.empty()) {
    option["transmission"] = folly::dynamic::object("httpHeader", std::move(header));
  }
  if (source.prebuffer > 0) {
    openQuestion(player, "minBufferForPlayback not sent: bufferControl counts bytes, not seconds");
  }
  if (source.maxWidth > 0 && source.maxHeight > 0) {
    option["adaptiveStreaming"] = folly::dynamic::object("maxWidth", source.maxWidth)("maxHeight", source.maxHeight);
  }
  folly::dynamic payload = folly::dynamic::object("option", std::move(option));
  if (lowered(source.uri).find("m3u8") != std::string::npos) {
    payload["mediaTransportType"] = "HLS";
    openQuestion(player, "mediaTransportType HLS sent beside option, as LG's plugin does");
  }
  return folly::dynamic::object("uri", source.uri)("type", "media")("payload", std::move(payload));
}

// OSE needs only the id; plx-native adds the flag on the releases it played on.
folly::dynamic subscribePayload(const std::string &mediaId) {
  return folly::dynamic::object("mediaId", mediaId)("subscribe", true);
}

// Open question 7. The web engine keeps audio at rate 1 only; nuvio always asks for it.
folly::dynamic playRatePayload(const std::string &mediaId, double rate) {
  return folly::dynamic::object("mediaId", mediaId)("playRate", rate)("audioOutput", true);
}

folly::dynamic volumePayload(const std::string &mediaId, double volume) {
  const auto level = static_cast<int>(std::lround(std::clamp(volume, 0.0, 1.0) * 100));
  return folly::dynamic::object("mediaId", mediaId)("volume", level)(
      "ease", folly::dynamic::object("duration", 0)("type", "Linear"));
}

// Inferred to be seconds: the web engine compares it with a duration in seconds.
double bufferedSeconds(double endTime) {
  return endTime;
}

// Inferred from what the codes mean: a refused command, an unknown subtitle
// file, a failed seek or rate change and the two buffer levels.
bool endsPlayback(int errorCode) {
  static constexpr std::array kSurvivable{101, 210, 700, 701, 61441, 61442};
  return std::ranges::find(kSurvivable, errorCode) == kSurvivable.end();
}

} // namespace rnwebos::expo::pipeline
