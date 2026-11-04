#include "../NetworkingTest.test.h"
#include "../blob/BlobStore.h"

#include <string>

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
using testing::serveOnce;
using testing::TemporaryFile;

} // namespace

TEST_CASE_FIXTURE(Network, "string, base64 and blob bodies reach the server as their bytes") {
  const auto blobId = BlobStore::shared().store("0123456789");
  for (const auto &[data, expected] : std::vector<std::pair<std::string, std::string>>{
           {"({string: 'h\\u00e9llo'})", "h\xC3\xA9llo"},
           {"({base64: 'AAEC'})", std::string("\0\1\2", 3)},
           {"({blob: {blobId: '" + blobId + "', offset: 2, size: 3}})", "234"},
       }) {
    auto served = serveOnce(server, kOk);
    send("POST", server.url("http"), 5, "[['Content-Type', 'application/octet-stream']]", data);
    const auto request = served.get();
    CHECK(request.head.starts_with("POST / HTTP/1.1\r\n"));
    CHECK(request.body == expected);
  }
  BlobStore::shared().remove(blobId);
}

TEST_CASE_FIXTURE(Network, "a FormData body goes as multipart/form-data, each part with its headers") {
  const TemporaryFile file("file body");
  auto served = serveOnce(server, kOk);
  send(
      "POST",
      server.url("http"),
      6,
      "[]",
      R"js(({formData: [
        {string: 'héllo', headers: [['content-disposition', 'form-data; name="title"']], fieldName: 'title'},
        {uri: 'data:text/plain;base64,aGk=', type: 'text/plain', name: 'note.txt',
         headers: [['content-disposition', 'form-data; name="note"; filename="note.txt"'], ['content-type', 'text/plain']]},
        {uri: ')js" + file.uri() + R"js(', headers: [['content-type', 'application/octet-stream'], ['content-disposition', 'form-data; name="upload"']]},
      ]}))js");
  const auto request = served.get();

  const auto type = headerOf(request.head, "Content-Type");
  REQUIRE(type.starts_with("multipart/form-data; boundary=RNTizenFormBoundary"));
  const auto boundary = type.substr(type.find('=') + 1);
  CHECK(request.body ==
        "--" + boundary + "\r\ncontent-disposition: form-data; name=\"title\"\r\n\r\nh\xC3\xA9llo\r\n" +
        "--" + boundary + "\r\ncontent-disposition: form-data; name=\"note\"; filename=\"note.txt\"\r\nContent-Type: text/plain\r\n\r\nhi\r\n" +
        "--" + boundary + "\r\ncontent-disposition: form-data; name=\"upload\"\r\nContent-Type: application/octet-stream\r\n\r\nfile body\r\n" +
        "--" + boundary + "--\r\n");
  CHECK(events.first("didCompleteNetworkResponse") == folly::dynamic::array(6, "", false));
}

TEST_CASE_FIXTURE(Network, "a FormData body keeps the request's multipart type, and an empty one sends only the closing boundary") {
  auto served = serveOnce(server, kOk);
  send("POST", server.url("http"), 7, "[['content-type', 'multipart/mixed']]", "({formData: []})");
  const auto request = served.get();

  const auto type = headerOf(request.head, "Content-Type");
  REQUIRE(type.starts_with("multipart/mixed; boundary="));
  CHECK(request.body == "--" + type.substr(type.find('=') + 1) + "--\r\n");
}

TEST_CASE_FIXTURE(Network, "a uri body sends the file with the request's own type") {
  const TemporaryFile file("uploaded");
  auto served = serveOnce(server, kOk);
  send("PUT", server.url("http"), 8, "[['Content-Type', 'text/plain']]", "({uri: '" + file.uri() + "'})");
  const auto request = served.get();

  CHECK(request.head.starts_with("PUT / HTTP/1.1\r\n"));
  CHECK(headerOf(request.head, "Content-Type") == "text/plain");
  CHECK(request.body == "uploaded");
}

TEST_CASE_FIXTURE(Network, "a uri body without a type fails the request") {
  CHECK(refusal("[]", "({uri: 'data:,x'})") == "Payload is set but no content-type header specified");
}

TEST_CASE_FIXTURE(Network, "a uri body that names no readable file fails the request") {
  CHECK(refusal("[['Content-Type', 'text/plain']]", "({uri: 'content://media/1'})") ==
        "Could not retrieve file for uri content://media/1");
}

TEST_CASE_FIXTURE(Network, "a FormData body under a type that is not multipart fails the request") {
  CHECK(refusal("[['Content-Type', 'text/plain']]", "({formData: []})") == "Invalid media type for FormData: text/plain");
}

TEST_CASE_FIXTURE(Network, "a FormData part that is not an object fails the request") {
  CHECK(refusal("[]", "({formData: ['text']})") == "Unrecognized FormData part.");
}

TEST_CASE_FIXTURE(Network, "a FormData part without headers fails the request") {
  CHECK(refusal("[]", "({formData: [{string: 'x'}]})") == "Missing or invalid header format for FormData part.");
}

TEST_CASE_FIXTURE(Network, "a FormData part header holding a line break fails the request") {
  CHECK(refusal("[]", R"js(({formData: [{string: 'x', headers: [['content-disposition', 'form-data; name="a"\r\nX-Injected: 1']]}]}))js") ==
        "Invalid header in FormData part.");
}

TEST_CASE_FIXTURE(Network, "a binary FormData part without a type fails the request") {
  CHECK(refusal("[]", "({formData: [{uri: 'data:,x', headers: [['content-disposition', 'form-data; name=\"f\"']]}]})") ==
        "Binary FormData part needs a content-type header.");
}

TEST_CASE_FIXTURE(Network, "a FormData part with neither a string nor a uri fails the request") {
  CHECK(refusal("[]", "({formData: [{headers: []}]})") == "Unrecognized FormData part.");
}

TEST_CASE_FIXTURE(Network, "a binary FormData part whose file cannot be read fails the request") {
  CHECK(refusal("[]", "({formData: [{uri: 'file:///no/such/file', headers: [['content-type', 'text/plain']]}]})") ==
        "Could not retrieve file for uri file:///no/such/file");
}

} // namespace rntizen
