#include "LunaBus.h"

#include "../threading/MainLoop.test.h"
#include "FakeLuna.test.h"

#include <doctest/doctest.h>

#include <chrono>
#include <string>
#include <vector>

namespace rnwebos {

namespace {

constexpr const char *kSystemInfo = "luna://com.webos.service.tv.systemproperty/getSystemInfo";
constexpr const char *kSettings = "luna://com.webos.settingsservice/getSystemSettings";

auto soon() {
  return std::chrono::steady_clock::now() + std::chrono::milliseconds(20);
}

} // namespace

TEST_CASE("a call sends its payload as JSON and hands the parsed reply to the main thread") {
  const testing::MainLoopScope mainLoop;
  testing::FakeLuna luna;
  luna.answer(kSystemInfo, R"({"returnValue":true,"modelName":"WEBOS1","firmwareVersion":"3.00.00"})");
  LunaBus bus(luna.transport());
  std::vector<std::string> models;

  CHECK(bus.isAvailable());
  CHECK(bus.call(
      kSystemInfo,
      folly::dynamic::object("keys", folly::dynamic::array("modelName")),
      [&models](const folly::dynamic &reply) { models.push_back(reply["modelName"].getString()); }));
  CHECK(models.empty());
  testing::FakeLuna::deliver();

  CHECK(models == std::vector<std::string>{"WEBOS1"});
  REQUIRE(luna.calls().size() == 1);
  CHECK(luna.calls()[0].payload == folly::dynamic::object("keys", folly::dynamic::array("modelName")));
}

TEST_CASE("an unavailable bus and a refused call answer nothing") {
  testing::FakeLuna luna;
  luna.refuse(kSettings);
  LunaBus bus(luna.transport());
  CHECK_FALSE(bus.call(kSettings, folly::dynamic::object()));
  CHECK(bus.ask(kSettings, folly::dynamic::object()) == nullptr);
  CHECK(bus.subscribe(kSettings, folly::dynamic::object(), [](const folly::dynamic &) {}) == nullptr);

  luna.setAvailable(false);
  CHECK_FALSE(bus.isAvailable());
  CHECK_FALSE(bus.call(kSystemInfo, folly::dynamic::object()));
  CHECK(bus.ask(kSystemInfo, folly::dynamic::object()) == nullptr);
  CHECK_FALSE(bus.callAndWait(kSystemInfo, folly::dynamic::object(), std::chrono::milliseconds(1)).has_value());
  CHECK(luna.calls().empty());
}

TEST_CASE("ask keeps the reply for whoever waits, and gives up at the deadline") {
  testing::FakeLuna luna;
  LunaBus bus(luna.transport());

  const auto unanswered = bus.ask(kSystemInfo, folly::dynamic::object());
  REQUIRE(unanswered != nullptr);
  CHECK_FALSE(unanswered->waitUntil(soon()).has_value());

  luna.answer(kSystemInfo, R"({"returnValue":true,"sdkVersion":"1.05.00"})");
  const auto answered = bus.ask(kSystemInfo, folly::dynamic::object());
  REQUIRE(answered != nullptr);
  const auto reply = answered->waitUntil(soon());
  REQUIRE(reply.has_value());
  CHECK((*reply)["sdkVersion"] == "1.05.00");
  CHECK(bus.callAndWait(kSystemInfo, folly::dynamic::object(), std::chrono::milliseconds(20)).has_value());
}

TEST_CASE("a reply that is not JSON, or too large, reads as a failed call") {
  testing::FakeLuna luna;
  LunaBus bus(luna.transport());

  luna.answer(kSystemInfo, "<html>");
  auto reply = bus.ask(kSystemInfo, folly::dynamic::object())->waitUntil(soon());
  REQUIRE(reply.has_value());
  CHECK((*reply)["returnValue"] == false);
  CHECK((*reply)["errorText"] == "The reply was not JSON");

  luna.answer(kSystemInfo, "\"" + std::string(4 * 1024 * 1024, 'a') + "\"");
  reply = bus.ask(kSystemInfo, folly::dynamic::object())->waitUntil(soon());
  REQUIRE(reply.has_value());
  CHECK((*reply)["errorText"] == "The reply was too large");
}

TEST_CASE("a subscription gets every reply until it is destroyed") {
  const testing::MainLoopScope mainLoop;
  testing::FakeLuna luna;
  luna.answer(kSettings, R"({"subscribed":false,"category":"option","settings":{"audioGuidance":"off"},"returnValue":true})");
  LunaBus bus(luna.transport());
  std::vector<std::string> values;

  auto subscription = bus.subscribe(
      kSettings,
      folly::dynamic::object("category", "option")("keys", folly::dynamic::array("audioGuidance"))("subscribe", true),
      [&values](const folly::dynamic &reply) { values.push_back(reply["settings"]["audioGuidance"].getString()); });
  REQUIRE(subscription != nullptr);
  testing::FakeLuna::deliver();
  luna.publish(kSettings, R"({"category":"option","settings":{"audioGuidance":"on"},"returnValue":true})");
  CHECK(values == std::vector<std::string>{"off", "on"});
  CHECK(luna.isSubscribed(kSettings));

  subscription.reset();
  CHECK_FALSE(luna.isSubscribed(kSettings));
  luna.publish(kSettings, R"({"settings":{"audioGuidance":"off"},"returnValue":true})");
  CHECK(values.size() == 2);
}

TEST_CASE("a reply already on its way to the main thread is dropped once its subscription is destroyed") {
  const testing::MainLoopScope mainLoop;
  testing::FakeLuna luna;
  luna.answer(kSettings, R"({"settings":{"audioGuidance":"off"},"returnValue":true})");
  LunaBus bus(luna.transport());
  int replies = 0;

  auto subscription = bus.subscribe(kSettings, folly::dynamic::object(), [&replies](const folly::dynamic &) { ++replies; });
  subscription.reset();
  testing::FakeLuna::deliver();

  CHECK(replies == 0);
}

} // namespace rnwebos
