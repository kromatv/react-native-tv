#include "BlobModule.h"
#include "BlobStore.h"

#include "../TurboModuleTest.test.h"
#include "../websocket/WebSocketSockets.h"

#include <doctest/doctest.h>
#include <jsi/instrumentation.h>

#include <string>

namespace rntizen {

namespace {

using facebook::jsi::JSError;
using facebook::jsi::String;
using testing::call;
using testing::evaluate;

struct Blobs {
  std::unique_ptr<facebook::jsi::Runtime> runtime = testing::makeRuntime();
  std::shared_ptr<BlobModule> module =
      std::make_shared<BlobModule>(std::make_shared<testing::QueuedInvoker>(), ModuleContext{});
};

} // namespace

TEST_CASE_FIXTURE(Blobs, "BlobModule answers blob: URLs with no host, as iOS does") {
  auto &rt = *runtime;
  CHECK(testing::json(rt, call(rt, *module, "getConstants")) == R"({"BLOB_URI_HOST":null,"BLOB_URI_SCHEME":"blob"})");
  CHECK(call(rt, *module, "addNetworkingHandler").isUndefined());
}

TEST_CASE_FIXTURE(Blobs, "createFromParts joins string parts and slices of other blobs under the id JavaScript chose") {
  auto &rt = *runtime;
  const auto source = BlobStore::shared().store("0123456789");
  call(
      rt,
      *module,
      "createFromParts",
      evaluate(rt, "[{type: 'string', data: 'h\\u00e9llo '}, {type: 'blob', data: {blobId: '" + source + "', offset: 2, size: 3}}]"),
      String::createFromUtf8(rt, "joined"));

  CHECK(BlobStore::shared().resolve("joined", 0, std::nullopt) == "h\xC3\xA9llo 234");
  call(rt, *module, "release", String::createFromUtf8(rt, "joined"));
  CHECK_FALSE(BlobStore::shared().resolve("joined", 0, std::nullopt).has_value());
  BlobStore::shared().remove(source);
}

TEST_CASE_FIXTURE(Blobs, "createFromParts refuses a part of an unknown type or a blob that is gone") {
  auto &rt = *runtime;
  auto create = [&](const std::string &parts) {
    call(rt, *module, "createFromParts", evaluate(rt, parts), String::createFromUtf8(rt, "refused"));
  };

  CHECK(testing::jsErrorMessage([&] { create("[{type: 'buffer', data: 'x'}]"); }) == "Invalid type for blob: buffer");
  CHECK(testing::jsErrorMessage([&] { create("[{type: 'blob', data: {blobId: 'gone'}}]"); }) == "The specified blob is invalid");
  CHECK_FALSE(BlobStore::shared().resolve("refused", 0, std::nullopt).has_value());
}

TEST_CASE_FIXTURE(Blobs, "a WebSocket delivers blobs between addWebSocketHandler and removeWebSocketHandler") {
  auto &rt = *runtime;
  call(rt, *module, "addWebSocketHandler", 77);
  CHECK(WebSocketSockets::shared().deliversBlobs(77));
  call(rt, *module, "removeWebSocketHandler", 77);
  CHECK_FALSE(WebSocketSockets::shared().deliversBlobs(77));
  CHECK(call(rt, *module, "sendOverSocket", evaluate(rt, "({blobId: 'none'})"), 77).isUndefined());
}

TEST_CASE_FIXTURE(Blobs, "the blob collector JavaScript holds keeps the blob's bytes until the runtime finalizes it") {
  auto &rt = *runtime;
  facebook::react::TurboModuleWithJSIBindings::installJSIBindings(module, rt);
  const auto id = BlobStore::shared().store("collected");

  CHECK(evaluate(rt, "__blobCollectorProvider(42)").isNull());
  CHECK(evaluate(rt, "__blobCollectorProvider()").isNull());
  evaluate(rt, "globalThis.collector = __blobCollectorProvider('" + id + "')");
  CHECK(evaluate(rt, "typeof globalThis.collector").asString(rt).utf8(rt) == "object");
  rt.instrumentation().collectGarbage("test");
  CHECK(BlobStore::shared().resolve(id, 0, std::nullopt) == "collected");

  runtime.reset();
  CHECK_FALSE(BlobStore::shared().resolve(id, 0, std::nullopt).has_value());
}

} // namespace rntizen
