#include "TvState.h"

#include <doctest/doctest.h>

#include <string>

namespace rntizen {

TEST_CASE("TvState counts command and search listeners apart") {
  TvState state(true, std::nullopt);

  state.addListener("search", 1);
  state.addListener("command", 2);
  state.addListener("command", -2);

  CHECK(state.listening("search"));
  CHECK_FALSE(state.listening("command"));
}

TEST_CASE("TvState keeps the voice configuration it was made with") {
  const TvState configured(true, "myapp://search?q={query}");
  const TvState bare(false, std::nullopt);

  CHECK(configured.voiceConfigured());
  CHECK(configured.searchUrl() == "myapp://search?q={query}");
  CHECK_FALSE(bare.voiceConfigured());
  CHECK_FALSE(bare.searchUrl().has_value());
}

TEST_CASE("TvState keeps what the shell and JavaScript last reported") {
  TvState state(true, std::nullopt);

  CHECK_FALSE(state.voiceAvailable());
  CHECK_FALSE(state.launchedFromTile());
  CHECK_FALSE(state.screen().has_value());
  CHECK(state.previewPublisher() == nullptr);

  state.setVoiceAvailable(true);
  state.setLaunchedFromTile(true);
  state.setScreen("player");
  state.setPreviewPublisher([](const std::string &json) { return static_cast<int>(json.size()); });

  CHECK(state.voiceAvailable());
  CHECK(state.launchedFromTile());
  CHECK(state.screen() == "player");
  CHECK(state.previewPublisher()("{}") == 2);
}

} // namespace rntizen
