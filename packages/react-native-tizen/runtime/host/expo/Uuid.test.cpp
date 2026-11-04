#include "Uuid.h"

#include <doctest/doctest.h>

#include <regex>
#include <set>

namespace rntizen::expo {

TEST_CASE("uuidV4 is a random RFC 4122 version 4 UUID in lowercase") {
  const std::regex layout("[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}");
  std::set<std::string> seen;
  for (int index = 0; index < 64; ++index) {
    const auto uuid = uuidV4();
    CHECK(std::regex_match(uuid, layout));
    seen.insert(uuid);
  }
  CHECK(seen.size() == 64);
}

TEST_CASE("uuidV5 hashes the name in its namespace, as RFC 4122 section 4.3 does") {
  constexpr auto dns = "6ba7b810-9dad-11d1-80b4-00c04fd430c8";
  CHECK(uuidV5("hello", dns) == "9342d47a-1bab-5709-9869-c840b2eac501");
  CHECK(uuidV5("python.org", dns) == "886313e1-3b8a-5372-9b90-0c9aee199e5d");
  CHECK(uuidV5("hello", "6BA7B810-9DAD-11D1-80B4-00C04FD430C8") == uuidV5("hello", dns));
  CHECK(uuidV5("", dns) == "4ebd0208-8328-5d69-8c44-ec50939c0967");
}

TEST_CASE("uuidV5 refuses a namespace that is not a UUID") {
  CHECK_FALSE(uuidV5("hello", "").has_value());
  CHECK_FALSE(uuidV5("hello", "6ba7b810-9dad-11d1-80b4-00c04fd430c").has_value());
  CHECK_FALSE(uuidV5("hello", "6ba7b810x9dad-11d1-80b4-00c04fd430c8").has_value());
  CHECK_FALSE(uuidV5("hello", "6ba7b81g-9dad-11d1-80b4-00c04fd430c8").has_value());
  CHECK_FALSE(uuidV5("hello", "6ba7b810-9dad-11d1-80b4-00c04fd430c8-").has_value());
}

} // namespace rntizen::expo
