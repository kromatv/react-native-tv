#include "PackagerMessage.h"

#include <doctest/doctest.h>

#include <folly/json.h>

#include <string>

namespace rntizen {

TEST_CASE("readPackagerMessage takes the reload and devMenu broadcasts") {
  CHECK(readPackagerMessage(R"({"version":2,"method":"reload"})").command == PackagerCommand::Reload);
  CHECK(readPackagerMessage(R"({"version":2,"method":"devMenu","params":null})").command == PackagerCommand::DevMenu);
}

TEST_CASE("readPackagerMessage answers a request it has no handler for") {
  const auto message = readPackagerMessage(R"({"version":2,"id":7,"method":"captureHeap"})");

  CHECK_FALSE(message.command.has_value());
  REQUIRE(message.reply.has_value());
  const auto reply = folly::parseJson(*message.reply);
  CHECK(reply["version"] == 2);
  CHECK(reply["id"] == 7);
  CHECK(reply["error"] == "No request handler for method: captureHeap");
}

TEST_CASE("readPackagerMessage answers a request that names no method") {
  const auto message = readPackagerMessage(R"({"version":2,"id":"a"})");

  REQUIRE(message.reply.has_value());
  CHECK(folly::parseJson(*message.reply)["error"] == "No method provided");
}

TEST_CASE("readPackagerMessage ignores notifications it does not know") {
  const auto message = readPackagerMessage(R"({"version":2,"method":"rsc-reload"})");

  CHECK_FALSE(message.command.has_value());
  CHECK_FALSE(message.reply.has_value());
}

TEST_CASE("readPackagerMessage ignores another protocol version, text that is not a JSON object, and a huge message") {
  CHECK_FALSE(readPackagerMessage(R"({"version":1,"method":"reload"})").command.has_value());
  CHECK_FALSE(readPackagerMessage(R"({"method":"reload"})").command.has_value());
  CHECK_FALSE(readPackagerMessage("reload").command.has_value());
  CHECK_FALSE(readPackagerMessage("[2]").command.has_value());
  const auto huge = std::string(70 * 1024, ' ') + R"({"version":2,"method":"reload"})";
  CHECK_FALSE(readPackagerMessage(huge).command.has_value());
}

} // namespace rntizen
