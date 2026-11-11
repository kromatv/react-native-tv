#include "NetworkingTest.test.h"

#include "blob/BlobStore.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

namespace rntizen {

namespace {

using namespace std::chrono_literals;
using facebook::jsi::String;
using facebook::jsi::Value;
using testing::headerOf;
using testing::kHello;
using testing::kOk;
using testing::LoopbackServer;
using testing::Network;
using testing::readRequest;
using testing::serveOnce;
using testing::TemporaryFile;

} // namespace

TEST_CASE_FIXTURE(Network, "a text request reports the response, then its body, then its completion") {
  auto served = serveOnce(server, kHello);
  send("GET", server.url("http", "/path?q=1"), 1, "[['X-Test', 'yes']]", "({trackingName: 'unknown'})");
  const auto seen = events.waitFor(3);
  const auto request = served.get();

  REQUIRE(seen.size() == 3);
  CHECK(seen[0].name == "didReceiveNetworkResponse");
  CHECK(seen[0].payload[0] == 1);
  CHECK(seen[0].payload[1] == 200);
  CHECK(seen[0].payload[2]["Content-Type"] == "text/plain; charset=utf-8");
  CHECK(seen[0].payload[2]["X-Multi"] == "a, b");
  CHECK(seen[0].payload[3] == server.url("http", "/path?q=1"));
  CHECK(seen[1].name == "didReceiveNetworkData");
  CHECK(seen[1].payload == folly::dynamic::array(1, "hello"));
  CHECK(seen[2].name == "didCompleteNetworkResponse");
  CHECK(seen[2].payload == folly::dynamic::array(1, "", false));
  CHECK(request.head.starts_with("GET /path?q=1 HTTP/1.1\r\n"));
  CHECK(headerOf(request.head, "X-Test") == "yes");
}

TEST_CASE_FIXTURE(Network, "a base64 response arrives encoded, and an incremental text one as one chunk") {
  auto first = serveOnce(server, kHello);
  send("GET", server.url("http"), 2, "[]", "({})", "base64");
  CHECK(events.first("didReceiveNetworkData") == folly::dynamic::array(2, "aGVsbG8="));
  first.get();
  events.waitFor(3);

  auto second = serveOnce(server, kHello);
  send("GET", server.url("http"), 3, "[]", "({})", "text", true);
  CHECK(events.first("didReceiveNetworkIncrementalData") == folly::dynamic::array(3, "hello", 5, 5));
  second.get();
}

TEST_CASE_FIXTURE(Network, "a blob response is stored, and JavaScript gets its BlobData with the response's type") {
  auto served = serveOnce(server, kHello);
  send("GET", server.url("http"), 4, "[]", "({})", "blob");
  const auto data = events.first("didReceiveNetworkData");
  served.get();

  REQUIRE(data.isArray());
  const auto &blob = data[1];
  CHECK(blob["offset"] == 0);
  CHECK(blob["size"] == 5);
  CHECK(blob["type"] == "text/plain; charset=utf-8");
  CHECK(BlobStore::shared().resolve(blob["blobId"].asString(), 0, std::nullopt) == "hello");
  BlobStore::shared().remove(blob["blobId"].asString());
}

TEST_CASE_FIXTURE(Network, "a blob: URL is answered from the blob store, by the range its query names") {
  const auto blobId = BlobStore::shared().store("0123456789");
  send("GET", "blob:" + blobId + "?offset=2&size=3", 10, "[]", "({})");
  const auto seen = events.waitFor(3);

  REQUIRE(seen.size() == 3);
  CHECK(seen[0].payload == folly::dynamic::array(10, 200, folly::dynamic::object(), "blob:" + blobId + "?offset=2&size=3"));
  CHECK(seen[1].payload == folly::dynamic::array(10, "234"));
  CHECK(seen[2].payload == folly::dynamic::array(10, "", false));

  send("GET", "blob://localhost/" + blobId + "?size=x&offset=-1&other=1", 11, "[]", "({})", "base64");
  CHECK(events.waitFor(6)[4].payload == folly::dynamic::array(11, "MDEyMzQ1Njc4OQ=="));
  BlobStore::shared().remove(blobId);
}

TEST_CASE_FIXTURE(Network, "a blob: URL that names no blob fails the request") {
  send("GET", "blob:gone", 12, "[]", "({})");
  CHECK(events.first("didCompleteNetworkResponse") == folly::dynamic::array(12, "The blob blob:gone does not exist", false));
}

TEST_CASE_FIXTURE(Network, "a request that takes longer than its timeout completes as timed out") {
  std::promise<void> finished;
  auto held = std::async(std::launch::async, [this, done = finished.get_future()] {
    const int socket = server.accept(5s);
    readRequest(socket);
    done.wait_for(5s);
    ::close(socket);
  });
  send("GET", server.url("http"), 13, "[]", "({})", "text", false, 200);
  const auto completed = events.first("didCompleteNetworkResponse");
  finished.set_value();
  held.get();

  REQUIRE(completed.isArray());
  CHECK(completed[0] == 13);
  CHECK_FALSE(completed[1].asString().empty());
  CHECK(completed[2] == true);
}

TEST_CASE_FIXTURE(Network, "an aborted request closes its connection and reports nothing more") {
  std::promise<void> received;
  auto closed = std::async(std::launch::async, [this, &received] {
    const int socket = server.accept(5s);
    readRequest(socket);
    received.set_value();
    char byte = 0;
    const auto got = ::recv(socket, &byte, 1, 0);
    ::close(socket);
    return got;
  });
  send("GET", server.url("http"), 14, "[]", "({})");
  received.get_future().wait_for(5s);
  testing::call(*runtime, *module, "abortRequest", 14);

  CHECK(closed.get() == 0);
  std::this_thread::sleep_for(100ms);
  CHECK(events.names().empty());
}

TEST_CASE_FIXTURE(Network, "a request to a port nothing listens on completes with libcurl's error") {
  send("GET", "http://127.0.0.1:1/", 15, "[]", "({})");
  const auto completed = events.first("didCompleteNetworkResponse");
  REQUIRE(completed.isArray());
  CHECK(completed[0] == 15);
  CHECK_FALSE(completed[1].asString().empty());
  CHECK(completed[2] == false);
}

TEST_CASE_FIXTURE(Network, "clearCookies calls back on the JavaScript thread with true") {
  auto &rt = *runtime;
  auto calls = std::make_shared<std::vector<std::string>>();
  testing::call(rt, *module, "clearCookies", testing::recorder(rt, calls));
  const auto giveUp = std::chrono::steady_clock::now() + 5s;
  while (calls->empty() && std::chrono::steady_clock::now() < giveUp) {
    invoker->run(rt);
    std::this_thread::sleep_for(1ms);
  }
  CHECK(*calls == std::vector<std::string>{"true"});
  CHECK(testing::call(rt, *module, "addListener", String::createFromUtf8(rt, "didReceiveNetworkData")).isUndefined());
  CHECK(testing::call(rt, *module, "removeListeners", 1).isUndefined());
}

} // namespace rntizen
