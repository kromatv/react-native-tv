#include "ImageLoaderModule.h"

#include "TurboModuleTest.test.h"

#include <doctest/doctest.h>

namespace rntizen {

using facebook::jsi::String;
using testing::call;
using testing::evaluate;
using testing::settled;

TEST_CASE("ImageLoader prefetches and queries trivially and refuses getSize") {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  ImageLoaderModule module(std::make_shared<testing::QueuedInvoker>(), ModuleContext{});
  const auto uri = String::createFromUtf8(rt, "https://example.com/a.png");

  CHECK(testing::json(rt, call(rt, module, "getConstants")) == "{}");
  CHECK(settled(rt, call(rt, module, "getSize", uri)) == "rejected: Image.getSize is not supported on Tizen yet");
  CHECK(settled(rt, call(rt, module, "getSizeWithHeaders", uri, evaluate(rt, "({})"))) ==
        "rejected: Image.getSize is not supported on Tizen yet");
  CHECK(settled(rt, call(rt, module, "prefetchImage", uri, 1)) == "resolved: true");
  CHECK(settled(rt, call(rt, module, "queryCache", evaluate(rt, "['a', 'b']"))) == "resolved: {}");
  CHECK(call(rt, module, "abortRequest", 1).isUndefined());
}

} // namespace rntizen
