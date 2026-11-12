#include "BlobReference.h"
#include "BlobStore.h"

#include "../TurboModuleTest.test.h"

#include <doctest/doctest.h>

#include <regex>
#include <set>

namespace rntizen {

TEST_CASE("the blob store keeps bytes under random version 4 ids") {
  auto &store = BlobStore::shared();
  const std::regex layout("[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}");
  std::set<std::string> ids;
  for (int index = 0; index < 32; ++index) {
    const auto id = store.store("x");
    CHECK(std::regex_match(id, layout));
    ids.insert(id);
    store.remove(id);
  }
  CHECK(ids.size() == 32);
}

TEST_CASE("the blob store answers a range of a blob, and nothing for a range outside it") {
  auto &store = BlobStore::shared();
  const auto id = store.store("hello world");

  CHECK(store.size(id) == 11);
  CHECK(store.resolve(id, 0, std::nullopt) == "hello world");
  CHECK(store.resolve(id, 6, std::nullopt) == "world");
  CHECK(store.resolve(id, 0, 5) == "hello");
  CHECK(store.resolve(id, 11, std::nullopt) == "");
  CHECK_FALSE(store.resolve(id, 12, std::nullopt).has_value());
  CHECK_FALSE(store.resolve(id, 6, 6).has_value());
  CHECK_FALSE(store.resolve("unknown", 0, std::nullopt).has_value());
  CHECK(store.size("unknown") == 0);

  store.store(id, "replaced");
  CHECK(store.resolve(id, 0, std::nullopt) == "replaced");
  store.remove(id);
  store.remove(id);
  CHECK_FALSE(store.resolve(id, 0, std::nullopt).has_value());
}

TEST_CASE("a BlobData from JavaScript names its blob, and a missing or negative offset or size means all of it") {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  const auto id = BlobStore::shared().store("abcdef");
  auto read = [&rt](const std::string &source) { return readBlobReference(rt, testing::evaluate(rt, source).asObject(rt)); };

  const auto full = read("({blobId: '" + id + "'})");
  CHECK(full.blobId == id);
  CHECK(full.offset == 0);
  CHECK_FALSE(full.size.has_value());
  CHECK(full.type.empty());
  CHECK(resolveBlob(full) == "abcdef");

  const auto slice = read("({blobId: '" + id + "', offset: 2, size: 3, type: 'text/plain'})");
  CHECK(slice.offset == 2);
  CHECK(slice.size == 3);
  CHECK(slice.type == "text/plain");
  CHECK(resolveBlob(slice) == "cde");

  const auto negative = read("({blobId: '" + id + "', offset: -4, size: -1, type: 7})");
  CHECK(negative.offset == 0);
  CHECK_FALSE(negative.size.has_value());
  CHECK(negative.type.empty());

  CHECK(testing::jsErrorMessage([&] { read("({offset: 1})"); }) == "The specified blob does not contain a blobId");
  BlobStore::shared().remove(id);
}

} // namespace rntizen
