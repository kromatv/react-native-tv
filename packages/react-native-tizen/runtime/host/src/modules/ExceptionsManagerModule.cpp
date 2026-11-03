#include "ExceptionsManagerModule.h"

#include "PlatformModuleRegistry.h"

#include "../diagnostics/ErrorScreen.h"
#include "../logging/Log.h"

#include <jsi/JSIDynamic.h>

namespace rntizen {

namespace {

std::string fileName(const std::string &path) {
  const auto slash = path.find_last_of('/');
  return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::string describeFrame(const folly::dynamic &frame) {
  auto text = [&frame](const char *key) {
    auto value = frame.get_ptr(key);
    return value != nullptr && value->isString() ? value->getString() : std::string("?");
  };
  auto number = [&frame](const char *key) {
    auto value = frame.get_ptr(key);
    return value != nullptr && value->isNumber() ? std::to_string(value->asInt()) : std::string("?");
  };
  return text("methodName") + " (" + fileName(text("file")) + ":" + number("lineNumber") + ":" + number("column") + ")";
}

std::vector<std::string> describeStack(const folly::dynamic &stack) {
  std::vector<std::string> frames;
  if (!stack.isArray()) return frames;
  for (const auto &frame : stack) {
    if (frame.isObject()) frames.push_back(describeFrame(frame));
  }
  return frames;
}

std::string describe(const ErrorReport &report) {
  auto text = report.title + ": " + report.message;
  for (const auto &frame : report.stack) text += "\n  at " + frame;
  return text;
}

std::string stringField(const folly::dynamic &object, const char *key) {
  auto value = object.get_ptr(key);
  return value != nullptr && value->isString() ? value->getString() : std::string();
}

ErrorReport reportFor(const folly::dynamic &exception) {
  auto name = stringField(exception, "name");
  auto message = stringField(exception, "originalMessage");
  if (message.empty()) message = stringField(exception, "message");
  auto stack = exception.get_ptr("stack");
  return ErrorReport{
      .title = name.empty() ? "Error" : name,
      .message = message.empty() ? "Unknown JavaScript error" : message,
      .stack = stack != nullptr ? describeStack(*stack) : std::vector<std::string>{},
  };
}

} // namespace

ExceptionsManagerModule::ExceptionsManagerModule(
    std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
    const ModuleContext &context)
    : NativeExceptionsManagerCxxSpec(std::move(jsInvoker)), reportFatalError_(context.reportFatalError) {}

void ExceptionsManagerModule::report(const ErrorReport &report, bool isFatal) {
  log(isFatal ? LogLevel::Error : LogLevel::Warning, describe(report));
  if (isFatal && reportFatalError_) reportFatalError_(report);
}

void ExceptionsManagerModule::reportFatalException(
    facebook::jsi::Runtime &runtime,
    facebook::jsi::String message,
    facebook::jsi::Array stack,
    double) {
  report(
      ErrorReport{
          .title = "Error",
          .message = message.utf8(runtime),
          .stack = describeStack(facebook::jsi::dynamicFromValue(runtime, facebook::jsi::Value(runtime, stack))),
      },
      true);
}

void ExceptionsManagerModule::reportSoftException(
    facebook::jsi::Runtime &runtime,
    facebook::jsi::String message,
    facebook::jsi::Array stack,
    double) {
  report(
      ErrorReport{
          .title = "Error",
          .message = message.utf8(runtime),
          .stack = describeStack(facebook::jsi::dynamicFromValue(runtime, facebook::jsi::Value(runtime, stack))),
      },
      false);
}

void ExceptionsManagerModule::reportException(facebook::jsi::Runtime &runtime, facebook::jsi::Object data) {
  auto exception = facebook::jsi::dynamicFromValue(runtime, facebook::jsi::Value(runtime, data));
  auto isFatal = exception.getDefault("isFatal", false);
  report(reportFor(exception), isFatal.isBool() && isFatal.getBool());
}

void ExceptionsManagerModule::dismissRedbox(facebook::jsi::Runtime &) {}

RN_TIZEN_PLATFORM_MODULE("ExceptionsManager", ExceptionsManagerModule);

} // namespace rntizen
