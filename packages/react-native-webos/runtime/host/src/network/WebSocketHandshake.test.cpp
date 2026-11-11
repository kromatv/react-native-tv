#include "NetworkError.h"
#include "WebSocketHandshake.h"

#include <doctest/doctest.h>

#include <stdexcept>
#include <string>

namespace rntizen {

namespace {

constexpr std::string_view kKey = "dGhlIHNhbXBsZSBub25jZQ==";

std::string answer(std::string_view extra = "") {
  return "HTTP/1.1 101 Switching Protocols\r\nUpgrade: WebSocket\r\nConnection: keep-alive, Upgrade\r\n"
         "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n" +
      std::string(extra);
}

} // namespace

TEST_CASE("webSocketAccept answers RFC 6455's sample key") {
  CHECK(webSocketAccept(kKey) == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}

TEST_CASE("makeWebSocketKey is 16 fresh bytes in base64") {
  const auto key = makeWebSocketKey();

  CHECK(key.size() == 24);
  CHECK(key.ends_with("=="));
  CHECK(key != makeWebSocketKey());
}

TEST_CASE("parseWebSocketUrl maps ws and wss to what libcurl connects to") {
  const auto plain = parseWebSocketUrl("ws://example.com/chat?room=1");
  const auto secure = parseWebSocketUrl("wss://example.com:8443");
  const auto http = parseWebSocketUrl("HTTP://10.0.0.2:81/a");

  CHECK(plain.connectUrl == "http://example.com/");
  CHECK(plain.authority == "example.com");
  CHECK(plain.target == "/chat?room=1");
  CHECK(plain.origin == "http://example.com");
  CHECK(secure.connectUrl == "https://example.com:8443/");
  CHECK(secure.authority == "example.com:8443");
  CHECK(secure.target == "/");
  CHECK(http.origin == "http://10.0.0.2:81");
}

TEST_CASE("parseWebSocketUrl refuses other schemes and malformed URLs") {
  CHECK_THROWS_AS(parseWebSocketUrl("ftp://example.com"), std::invalid_argument);
  CHECK_THROWS_AS(parseWebSocketUrl("not a url"), std::invalid_argument);
}

TEST_CASE("buildWebSocketHandshake asks to upgrade and adds Origin and User-Agent only when missing") {
  const auto endpoint = parseWebSocketUrl("ws://example.com/chat");

  const auto request = buildWebSocketHandshake(endpoint, kKey, {"a", "b"}, {{"X-Id", "7"}});
  const auto own = buildWebSocketHandshake(endpoint, kKey, {}, {{"origin", "app://x"}, {"User-Agent", "me"}});

  CHECK(request.starts_with("GET /chat HTTP/1.1\r\nHost: example.com\r\n"));
  CHECK(request.find("Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n") != std::string::npos);
  CHECK(request.find("Sec-WebSocket-Protocol: a,b\r\n") != std::string::npos);
  CHECK(request.find("Origin: http://example.com\r\n") != std::string::npos);
  CHECK(request.find("User-Agent: react-native-tizen\r\n") != std::string::npos);
  CHECK(request.ends_with("X-Id: 7\r\n\r\n"));
  CHECK(own.find("Origin: http") == std::string::npos);
  CHECK(own.find("react-native-tizen") == std::string::npos);
  CHECK(own.find("Sec-WebSocket-Protocol") == std::string::npos);
}

TEST_CASE("buildWebSocketHandshake refuses a header that would start another line") {
  const auto endpoint = parseWebSocketUrl("ws://example.com/");

  CHECK_THROWS_AS(
      buildWebSocketHandshake(endpoint, kKey, {}, {{"X-Id", "7\r\nCookie: stolen"}}), std::invalid_argument);
  CHECK_THROWS_AS(buildWebSocketHandshake(endpoint, kKey, {}, {{"X Id", "7"}}), std::invalid_argument);
}

TEST_CASE("checkWebSocketHandshake returns the protocol the server picked") {
  CHECK(checkWebSocketHandshake(answer(), kKey, {}).empty());
  CHECK(checkWebSocketHandshake(answer("Sec-WebSocket-Protocol: b\r\n"), kKey, {"a", "b"}) == "b");
}

TEST_CASE("checkWebSocketHandshake names what is wrong with an answer") {
  CHECK_THROWS_WITH_AS(
      checkWebSocketHandshake("HTTP/1.1 403 Forbidden\r\n", kKey, {}),
      "Expected HTTP 101 response but was '403 Forbidden'",
      NetworkError);
  CHECK_THROWS_AS(checkWebSocketHandshake("garbage", kKey, {}), NetworkError);
  CHECK_THROWS_AS(checkWebSocketHandshake(answer(), "AAAAAAAAAAAAAAAAAAAAAA==", {}), NetworkError);
  CHECK_THROWS_AS(checkWebSocketHandshake(answer("Sec-WebSocket-Extensions: x\r\n"), kKey, {}), NetworkError);
  CHECK_THROWS_AS(checkWebSocketHandshake(answer("Sec-WebSocket-Protocol: c\r\n"), kKey, {"a"}), NetworkError);
  CHECK_THROWS_AS(
      checkWebSocketHandshake(
          "HTTP/1.1 101 OK\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n", kKey, {}),
      NetworkError);
}

} // namespace rntizen
