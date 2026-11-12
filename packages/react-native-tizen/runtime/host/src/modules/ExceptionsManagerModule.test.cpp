#include "ExceptionsManagerModule.h"

#include "TurboModuleTest.test.h"

#include "../diagnostics/ErrorScreen.h"
#include "../logging/Log.h"

#include <doctest/doctest.h>

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace rntizen {

namespace {

using testing::evaluate;

class RecordingSink final : public LogSink {
 public:
  void write(LogLevel level, std::string_view message) override {
    std::lock_guard lock(mutex);
    lines.emplace_back(level, std::string(message));
  }

  std::mutex mutex;
  std::vector<std::pair<LogLevel, std::string>> lines;
};

struct Exceptions {
  Exceptions() {
    setLogSink(sink);
    ModuleContext context{};
    context.reportFatalError = [this](const ErrorReport &report) { fatal = report; };
    module = std::make_shared<ExceptionsManagerModule>(std::make_shared<testing::QueuedInvoker>(), context);
  }
  ~Exceptions() {
    setLogSink(nullptr);
  }
  Exceptions(const Exceptions &) = delete;
  Exceptions &operator=(const Exceptions &) = delete;

  std::unique_ptr<facebook::jsi::Runtime> runtime = testing::makeRuntime();
  std::shared_ptr<RecordingSink> sink = std::make_shared<RecordingSink>();
  std::optional<ErrorReport> fatal;
  std::shared_ptr<ExceptionsManagerModule> module;
};

} // namespace

TEST_CASE_FIXTURE(Exceptions, "a fatal reportException takes the host's fatal path with the error's name, message and stack") {
  auto &rt = *runtime;
  testing::call(
      rt,
      *module,
      "reportException",
      evaluate(
          rt,
          R"js({name: "TypeError", message: "Cannot read x of undefined (extra)", originalMessage: "Cannot read x of undefined",
              isFatal: true, stack: [{methodName: "render", file: "http://host/index.bundle", lineNumber: 12, column: 7},
              "not a frame", {file: "app.js"}]})js"));

  REQUIRE(fatal.has_value());
  CHECK(fatal->title == "TypeError");
  CHECK(fatal->message == "Cannot read x of undefined");
  CHECK(fatal->stack == std::vector<std::string>{"render (index.bundle:12:7)", "? (app.js:?:?)"});
  REQUIRE(sink->lines.size() == 1);
  CHECK(sink->lines[0].first == LogLevel::Error);
  CHECK(sink->lines[0].second ==
        "TypeError: Cannot read x of undefined\n  at render (index.bundle:12:7)\n  at ? (app.js:?:?)");
}

TEST_CASE_FIXTURE(Exceptions, "a soft reportException only logs, and fills in what JavaScript left out") {
  auto &rt = *runtime;
  testing::call(rt, *module, "reportException", evaluate(rt, "({isFatal: false})"));
  testing::call(rt, *module, "reportException", evaluate(rt, R"({message: "plain", isFatal: "yes", stack: 3})"));

  CHECK_FALSE(fatal.has_value());
  REQUIRE(sink->lines.size() == 2);
  CHECK(sink->lines[0].first == LogLevel::Warning);
  CHECK(sink->lines[0].second == "Error: Unknown JavaScript error");
  CHECK(sink->lines[1].second == "Error: plain");
}

TEST_CASE_FIXTURE(Exceptions, "reportFatalException and reportSoftException differ only in taking the fatal path") {
  auto &rt = *runtime;
  const auto stack = R"([{methodName: "boot", file: "main.js", lineNumber: 3, column: 1}])";
  testing::call(
      rt, *module, "reportSoftException", facebook::jsi::String::createFromUtf8(rt, "soft"), evaluate(rt, stack), 1);
  CHECK_FALSE(fatal.has_value());
  testing::call(
      rt, *module, "reportFatalException", facebook::jsi::String::createFromUtf8(rt, "hard"), evaluate(rt, stack), 2);

  REQUIRE(fatal.has_value());
  CHECK(fatal->title == "Error");
  CHECK(fatal->message == "hard");
  CHECK(fatal->stack == std::vector<std::string>{"boot (main.js:3:1)"});
  REQUIRE(sink->lines.size() == 2);
  CHECK(sink->lines[0] == std::pair<LogLevel, std::string>{LogLevel::Warning, "Error: soft\n  at boot (main.js:3:1)"});
  CHECK(sink->lines[1].first == LogLevel::Error);
  CHECK(testing::call(rt, *module, "dismissRedbox").isUndefined());
}

TEST_CASE("a fatal report without a host to tell only logs") {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  ExceptionsManagerModule module(std::make_shared<testing::QueuedInvoker>(), ModuleContext{});
  CHECK(testing::call(rt, module, "reportException", evaluate(rt, R"({message: "late", isFatal: true})")).isUndefined());
}

} // namespace rntizen
