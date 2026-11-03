#include "TvState.h"

#include <doctest/doctest.h>

namespace rntizen {

TEST_CASE("TvState counts command and search listeners apart") {
  TvState state(true, std::nullopt);

  state.addListener("search", 1);
  state.addListener("command", 2);
  state.addListener("command", -2);

  CHECK(state.listening("search"));
  CHECK_FALSE(state.listening("command"));
}

} // namespace rntizen
