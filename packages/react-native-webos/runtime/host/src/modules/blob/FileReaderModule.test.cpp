#include "BlobStore.h"
#include "FileReaderModule.h"

#include "../TurboModuleTest.test.h"

#include <doctest/doctest.h>

#include <string>

namespace rntizen {

namespace {

using facebook::jsi::String;
using testing::call;
using testing::settled;

struct Reader {
  Reader() : module(std::make_shared<testing::QueuedInvoker>(), ModuleContext{}) {}
  ~Reader() {
    BlobStore::shared().remove(id);
  }
  Reader(const Reader &) = delete;
  Reader &operator=(const Reader &) = delete;

  std::string text(const std::string &encoding) {
    auto &rt = *runtime;
    return settled(rt, call(rt, module, "readAsText", blob(""), String::createFromUtf8(rt, encoding)));
  }

  facebook::jsi::Value blob(const std::string &more) {
    return testing::evaluate(*runtime, "({blobId: '" + id + "'" + more + "})");
  }

  std::unique_ptr<facebook::jsi::Runtime> runtime = testing::makeRuntime();
  FileReaderModule module;
  std::string id = BlobStore::shared().store("");
};

} // namespace

TEST_CASE_FIXTURE(Reader, "readAsText decodes UTF-8 by default and by every name of it") {
  BlobStore::shared().store(id, "caf\xC3\xA9 \xE2\x9C\x93");
  for (const auto *encoding : {"", "UTF-8", "utf-8", "utf8"}) {
    CHECK(text(encoding) == "resolved: \"caf\xC3\xA9 \xE2\x9C\x93\"");
  }
}

TEST_CASE_FIXTURE(Reader, "readAsText decodes ISO-8859-1 to UTF-8 and US-ASCII with replacement characters") {
  BlobStore::shared().store(id, "caf\xE9 \xFF");
  for (const auto *encoding : {"ISO-8859-1", "latin1", "L1"}) {
    CHECK(text(encoding) == "resolved: \"caf\xC3\xA9 \xC3\xBF\"");
  }
  for (const auto *encoding : {"US-ASCII", "ascii"}) {
    CHECK(text(encoding) == "resolved: \"caf\xEF\xBF\xBD \xEF\xBF\xBD\"");
  }
}

TEST_CASE_FIXTURE(Reader, "readAsText rejects an encoding it does not know and a blob that is gone") {
  auto &rt = *runtime;
  CHECK(text("UTF-16") == "rejected: Unsupported encoding UTF-16");
  CHECK(settled(rt, call(rt, module, "readAsText", blob(", offset: 99"), String::createFromUtf8(rt, ""))) ==
        "rejected: The specified blob is invalid");
}

TEST_CASE_FIXTURE(Reader, "readAsDataURL keeps the blob's type, or names it application/octet-stream") {
  auto &rt = *runtime;
  BlobStore::shared().store(id, "hi");
  CHECK(settled(rt, call(rt, module, "readAsDataURL", blob(", type: 'text/plain'"))) ==
        R"(resolved: "data:text/plain;base64,aGk=")");
  CHECK(settled(rt, call(rt, module, "readAsDataURL", blob(", offset: 1"))) ==
        R"(resolved: "data:application/octet-stream;base64,aQ==")");
  CHECK(settled(rt, call(rt, module, "readAsDataURL", blob(", size: 5"))) == "rejected: The specified blob is invalid");
}

} // namespace rntizen
