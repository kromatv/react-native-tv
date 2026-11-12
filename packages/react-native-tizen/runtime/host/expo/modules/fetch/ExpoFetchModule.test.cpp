#include "../../ExpoTestRuntime.test.h"
#include "../HttpTestServer.test.h"

#include "modules/blob/BlobStore.h"

#include <doctest/doctest.h>

#include <future>
#include <mutex>
#include <string>

namespace rntizen::expo {

namespace {

/// A server whose every answer waits until `open()` or the end of the test, so
/// a test can act while a request waits for its response. Its destructor opens
/// before the server stops, so a test that ends early never leaves it waiting.
class GatedServer final {
 public:
  explicit GatedServer(testing::HttpTestServer::Respond respond)
      : server_([released = released_, respond = std::move(respond)](const std::string &request) {
          released.wait();
          return respond(request);
        }) {}

  ~GatedServer() {
    open();
  }

  GatedServer(const GatedServer &) = delete;
  GatedServer &operator=(const GatedServer &) = delete;

  void open() {
    std::call_once(opened_, [this] { answer_.set_value(); });
  }

  std::string url(std::string_view path) const {
    return server_.url(path);
  }

  std::vector<std::string> requests() const {
    return server_.requests();
  }

 private:
  std::promise<void> answer_;
  std::shared_future<void> released_ = answer_.get_future().share();
  std::once_flag opened_;
  testing::HttpTestServer server_;
};

std::string echo(const std::string &request) {
  const auto path = request.substr(request.find(' ') + 1, request.find(' ', request.find(' ') + 1) - request.find(' ') - 1);
  if (path == "/redirect") return testing::httpResponse(302, "Found", "", "Location: /final\r\n");
  if (path == "/missing") return testing::httpResponse(404, "Not Found", "nothing here");
  const auto body = request.substr(request.find("\r\n\r\n") + 4);
  return testing::httpResponse(200, "OK", request.substr(0, request.find(' ')) + " " + path + " " + body, "X-Multi: a\r\nX-Multi: b\r\n");
}

void openFetch(testing::ExpoTestRuntime &expo, const std::string &base) {
  expo.eval(
      "globalThis.F = expo.modules.ExpoFetchModule; globalThis.base = '" + base +
      "';"
      "globalThis.res = new F.NativeResponse(); globalThis.req = new F.NativeRequest(res); globalThis.events = [];"
      "['didReceiveResponseData', 'didComplete', 'didFailWithError', 'readyForJSFinalization'].forEach(name =>"
      " res.addListener(name, value => events.push(value === undefined ? name"
      " : name + ':' + (value instanceof Uint8Array ? String.fromCharCode(...value) : value))));");
}

} // namespace

TEST_CASE("a GET through NativeRequest settles at the response and reads it as text") {
  testing::ExpoTestRuntime expo;
  testing::HttpTestServer server(echo);
  openFetch(expo, server.url("/"));

  CHECK(expo.settle("req.start(base + 'hello?x=1', { method: 'GET', headers: [['X-Test', 'yes'], ['bad'], [1, 2]] }, null)") ==
        "undefined");
  CHECK(expo.json("[res.status, res.statusText, res.url === base + 'hello?x=1', res.redirected, res.bodyUsed]") ==
        R"([200,"",true,false,false])");
  CHECK(expo.json("res._rawHeaders.filter(([name]) => name === 'X-Multi')") == R"([["X-Multi","a, b"]])");
  CHECK(expo.settle("res.text()") == R"("GET /hello?x=1 ")");
  CHECK(expo.json("res.bodyUsed") == "true");
  CHECK(expo.pumpUntil([&expo] { return expo.json("events") == R"(["readyForJSFinalization"])"; }));
  const auto requests = server.requests();
  REQUIRE(requests.size() == 1);
  CHECK(requests.front().find("X-Test: yes\r\n") != std::string::npos);
  CHECK(requests.front().find("bad") == std::string::npos);
}

TEST_CASE("a POST sends its method and bytes, and the body reads back as an ArrayBuffer") {
  testing::ExpoTestRuntime expo;
  testing::HttpTestServer server(echo);
  openFetch(expo, server.url("/"));

  CHECK(expo.settle("req.start(base + 'echo', { method: 'POST', headers: [['Content-Type', 'text/plain']] },"
                    " new Uint8Array([104, 105]))") == "undefined");
  CHECK(expo.settle("res.arrayBuffer().then(buffer => [buffer instanceof ArrayBuffer, String.fromCharCode(...new Uint8Array(buffer))])") ==
        R"([true,"POST /echo hi"])");
}

TEST_CASE("a redirected request reports where it ended and the status it got there") {
  testing::ExpoTestRuntime expo;
  testing::HttpTestServer server(echo);
  openFetch(expo, server.url("/"));
  CHECK(expo.settle("req.start(base + 'redirect', {}, null)") == "undefined");
  CHECK(expo.json("[res.status, res.redirected, res.url === base + 'final']") == "[200,true,true]");
  CHECK(expo.settle("res.text()") == R"("GET /final ")");

  expo.eval("globalThis.missing = new F.NativeResponse()");
  CHECK(expo.settle("new F.NativeRequest(missing).start(base + 'missing', {}, null)") == "undefined");
  CHECK(expo.json("missing.status") == "404");
  CHECK(expo.settle("missing.text()") == R"("nothing here")");
}

TEST_CASE("startStreaming after the response hands over the whole body at once") {
  testing::ExpoTestRuntime expo;
  testing::HttpTestServer server(echo);
  openFetch(expo, server.url("/"));
  CHECK(expo.settle("req.start(base + 'whole', {}, null)") == "undefined");
  CHECK(expo.settle("res.startStreaming().then(bytes => String.fromCharCode(...bytes))") == R"("GET /whole ")");
  CHECK(expo.json("res.bodyUsed") == "true");
}

TEST_CASE("startStreaming before the body arrived gets null, then the body and the end as events") {
  testing::ExpoTestRuntime expo;
  GatedServer server(echo);
  openFetch(expo, server.url("/"));
  expo.eval("globalThis.started = req.start(base + 'later', {}, null)");
  CHECK(expo.settle("res.startStreaming()") == "null");
  server.open();
  CHECK(expo.settle("started") == "undefined");
  CHECK(expo.pumpUntil([&expo] { return expo.json("events.length") == "3"; }));
  CHECK(expo.json("events") == R"(["didReceiveResponseData:GET /later ","didComplete","readyForJSFinalization"])");
}

TEST_CASE("cancelStreaming stops the body's events, though the response still ends") {
  testing::ExpoTestRuntime expo;
  GatedServer server(echo);
  openFetch(expo, server.url("/"));
  expo.eval("globalThis.started = req.start(base + 'quiet', {}, null)");
  CHECK(expo.settle("res.startStreaming()") == "null");
  expo.eval("res.cancelStreaming('not needed')");
  server.open();
  CHECK(expo.settle("started") == "undefined");
  CHECK(expo.pumpUntil([&expo] { return expo.json("events") == R"(["readyForJSFinalization"])"; }));
}

TEST_CASE("text() asked before the body arrived settles when it does") {
  testing::ExpoTestRuntime expo;
  GatedServer server(echo);
  openFetch(expo, server.url("/"));
  expo.eval("globalThis.started = req.start(base + 'pending', {}, null); globalThis.text = res.text()");
  CHECK(expo.pumpUntil([&expo] { return expo.json("res.bodyUsed") == "true"; }));
  server.open();
  CHECK(expo.settle("text") == R"("GET /pending ")");
  CHECK(expo.settle("started") == "undefined");
}

TEST_CASE("cancel while the server has not answered rejects start with the reason") {
  testing::ExpoTestRuntime expo;
  GatedServer server(echo);
  openFetch(expo, server.url("/"));
  expo.eval("globalThis.started = req.start(base + 'cancelled', {}, null)");
  CHECK(expo.pumpUntil([&server] { return server.requests().size() == 1; }));
  expo.eval("req.cancel()");
  CHECK_THROWS_WITH(expo.settle("started"), "ERR_FETCH_FAILED: The request was canceled");
  server.open();
}

TEST_CASE("a request cancelled before it started never reaches the server") {
  testing::ExpoTestRuntime expo;
  testing::HttpTestServer server(echo);
  openFetch(expo, server.url("/"));
  expo.eval("req.cancel()");
  CHECK_THROWS_WITH(expo.settle("req.start(base + 'late', {}, null)"), "ERR_FETCH_FAILED: The request was canceled");
  CHECK(server.requests().empty());
}

TEST_CASE("a streaming response that is cancelled reports the failure, and its body reads reject") {
  testing::ExpoTestRuntime expo;
  openFetch(expo, "http://127.0.0.1:0/");
  CHECK(expo.settle("res.startStreaming()") == "null");
  expo.eval("globalThis.unread = res.text(); req.cancel(); req.cancel()");
  CHECK_THROWS_WITH(expo.settle("unread"), "ERR_FETCH_FAILED: The request was canceled");
  CHECK(expo.pumpUntil([&expo] { return expo.json("events.length") == "2"; }));
  CHECK(expo.json("events") == R"(["didFailWithError:The request was canceled","readyForJSFinalization"])");
  CHECK_THROWS_WITH(expo.settle("res.arrayBuffer()"), "ERR_FETCH_FAILED: The request was canceled");
  CHECK(expo.settle("res.startStreaming()") == "null");
}

TEST_CASE("a request the server cannot be reached for rejects start") {
  testing::ExpoTestRuntime expo;
  openFetch(expo, "http://127.0.0.1:0/");
  try {
    expo.settle("req.start(base + 'nowhere', {}, null)");
    FAIL("an unreachable server resolved");
  } catch (const std::runtime_error &error) {
    CHECK(std::string(error.what()).starts_with("ERR_FETCH_FAILED: "));
  }
}

TEST_CASE("unstable_createBlobData stores the bytes in the host's BlobStore") {
  testing::ExpoTestRuntime expo;
  const auto id = expo.settle("expo.modules.ExpoFetchModule.unstable_createBlobData(new Uint8Array([104, 105]))");
  REQUIRE(id.size() > 2);
  const auto blobId = id.substr(1, id.size() - 2);
  CHECK(BlobStore::shared().resolve(blobId, 0, std::nullopt) == "hi");
  BlobStore::shared().remove(blobId);
}

} // namespace rntizen::expo
