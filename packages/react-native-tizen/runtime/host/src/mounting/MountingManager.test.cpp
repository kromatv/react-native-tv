#include "MountingManager.h"

#include <doctest/doctest.h>

#include <bit>
#include <cstdint>

namespace rntizen {

TEST_CASE("MountingManager drops a command for a view it has not mounted") {
  MountingManager manager(std::bit_cast<Evas *>(std::uintptr_t{0x1000}));

  CHECK_NOTHROW(manager.dispatchCommand(42, "focus", folly::dynamic::array()));
  CHECK(manager.findView(42) == nullptr);
}

} // namespace rntizen
