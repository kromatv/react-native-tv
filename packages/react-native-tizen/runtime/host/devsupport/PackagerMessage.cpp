#include "PackagerMessage.h"

#include "logging/Log.h"

#include <folly/json.h>

namespace rntizen {

namespace {

constexpr int kProtocolVersion = 2;
constexpr std::size_t kMaxMessageBytes = 64 * 1024;

std::optional<folly::dynamic> parseObject(std::string_view text) {
  if (text.size() > kMaxMessageBytes) return std::nullopt;
  try {
    auto message = folly::parseJson(text);
    if (message.isObject()) return message;
  } catch (const folly::json::parse_error &) {
    log(LogLevel::Warning, "The dev server sent a message that is not JSON");
  }
  return std::nullopt;
}

std::string errorReply(const folly::dynamic &id, const std::string &error) {
  return folly::toJson(folly::dynamic::object("version", kProtocolVersion)("id", id)("error", error));
}

} // namespace

PackagerMessage readPackagerMessage(std::string_view text) {
  const auto message = parseObject(text);
  if (!message.has_value()) return {};
  if (const auto *version = message->get_ptr("version");
      version == nullptr || !version->isInt() || version->getInt() != kProtocolVersion) {
    log(LogLevel::Warning, "The dev server speaks another version of the packager protocol");
    return {};
  }
  const auto *method = message->get_ptr("method");
  const auto name = method != nullptr && method->isString() ? method->getString() : std::string();
  if (name == "reload") return {.command = PackagerCommand::Reload, .reply = std::nullopt};
  if (name == "devMenu") return {.command = PackagerCommand::DevMenu, .reply = std::nullopt};
  const auto *id = message->get_ptr("id");
  if (id == nullptr || id->isNull()) return {};
  return {
      .command = std::nullopt,
      .reply = errorReply(*id, name.empty() ? "No method provided" : "No request handler for method: " + name),
  };
}

} // namespace rntizen
