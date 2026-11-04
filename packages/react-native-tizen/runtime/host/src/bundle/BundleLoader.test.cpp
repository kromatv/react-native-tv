#include "BundleLoader.h"

#include "../network/LoopbackServer.test.h"
#include "../network/NetworkError.h"

#include <doctest/doctest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

namespace rntizen {

namespace {

namespace fs = std::filesystem;
using namespace std::chrono_literals;
using testing::LoopbackServer;

std::thread answerOnce(const LoopbackServer &server, std::string response) {
  return std::thread([&server, response = std::move(response)] {
    const int socket = server.accept(5s);
    testing::readUntil(socket, "\r\n\r\n");
    testing::writeAll(socket, response);
    ::close(socket);
  });
}

} // namespace

TEST_CASE("loadBundleFile reads the packaged bundle and reports it as a file URL") {
  const auto file = fs::temp_directory_path() / "rn-tizen-bundle-test.bundle";
  std::ofstream(file, std::ios::binary) << "globalThis.ready = true;";

  const auto bundle = loadBundleFile(file.string());

  CHECK(std::string(bundle.script->c_str(), bundle.script->size()) == "globalThis.ready = true;");
  CHECK(bundle.sourceURL == "file://" + file.string());
  fs::remove(file);
}

TEST_CASE("loadBundleFile names a missing bundle") {
  CHECK_THROWS_AS(loadBundleFile("/nonexistent/index.bundle"), BundleError);
}

TEST_CASE("downloadBundle takes the dev server's bundle and reports it under its URL") {
  const LoopbackServer server;
  const auto url = server.url("http", "/index.bundle?platform=tizen");
  auto peer = answerOnce(server, "HTTP/1.1 200 OK\r\nContent-Length: 24\r\n\r\nglobalThis.ready = true;");

  const auto bundle = downloadBundle(url);
  peer.join();

  CHECK(std::string(bundle.script->c_str(), bundle.script->size()) == "globalThis.ready = true;");
  CHECK(bundle.sourceURL == url);
}

TEST_CASE("downloadBundle passes on the dev server's error") {
  const LoopbackServer server;
  const auto url = server.url("http", "/index.bundle");
  auto peer = answerOnce(server, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 12\r\n\r\nSyntaxError!");

  CHECK_THROWS_WITH_AS(downloadBundle(url), (url + " answered 500:\nSyntaxError!").c_str(), NetworkError);
  peer.join();
}

} // namespace rntizen
