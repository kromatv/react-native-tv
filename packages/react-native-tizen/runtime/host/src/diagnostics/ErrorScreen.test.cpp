#include "ErrorScreen.h"

#include <doctest/doctest.h>

#include <Ecore_Evas.h>

#include <string>
#include <vector>

namespace rntizen {

TEST_CASE("ErrorScreen shows a report whose text holds markup characters") {
  static const bool initialized = ecore_evas_init() > 0;
  REQUIRE(initialized);
  auto *canvas = ecore_evas_buffer_new(640, 360);
  const std::vector<std::string> stack(30, "at <anonymous> (index.bundle:1:2)");

  {
    ErrorScreen screen(ecore_evas_get(canvas));
    CHECK_FALSE(screen.isShown());

    screen.show({.title = "TypeError", .message = "a < b && c > d", .stack = stack}, 640, 360);

    CHECK(screen.isShown());
  }

  ecore_evas_free(canvas);
}

} // namespace rntizen
