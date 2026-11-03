#include "VoiceCommand.h"

#include "../logging/Log.h"

#include <folly/json.h>

#include <array>
#include <algorithm>

namespace rntizen {

namespace {

constexpr std::array kTransport{
    "play", "pause", "stop", "restart", "next", "previous", "fastForward", "rewind", "skip", "seek", "subtitles"};
constexpr std::array kOthers{"search", "selectTitle", "selectIndex"};
constexpr std::array kSearchFields{"utterance", "title", "genre", "cast", "contentType", "releasedFrom", "releasedTo"};

bool oneOf(const auto &names, const std::string &type) {
  return std::find(names.begin(), names.end(), type) != names.end();
}

double number(const folly::dynamic &object, const char *name) {
  const auto *field = object.get_ptr(name);
  if (field == nullptr || !field->isNumber()) throw std::invalid_argument(std::string("`") + name + "` must be a number");
  return field->asDouble();
}

std::string text(const folly::dynamic &object, const char *name) {
  const auto *field = object.get_ptr(name);
  if (field == nullptr || !field->isString()) throw std::invalid_argument(std::string("`") + name + "` must be a string");
  return field->getString();
}

VoiceCommand read(const folly::dynamic &json) {
  if (!json.isObject()) throw std::invalid_argument("not an object");
  VoiceCommand command{.type = text(json, "type")};
  if (!oneOf(kTransport, command.type) && !oneOf(kOthers, command.type)) {
    throw std::invalid_argument("unknown type " + command.type);
  }
  if (command.type == "skip") command.seconds = number(json, "seconds");
  if (command.type == "seek") command.position = number(json, "position");
  if (command.type == "selectIndex") command.index = static_cast<int>(number(json, "index"));
  if (command.type == "selectTitle") command.title = text(json, "title");
  if (command.type == "subtitles") {
    const auto *enabled = json.get_ptr("enabled");
    if (enabled == nullptr || !enabled->isBool()) throw std::invalid_argument("`enabled` must be a boolean");
    command.enabled = enabled->getBool();
  }
  if (command.type == "search") {
    for (const auto *field : kSearchFields) {
      const auto *value = json.get_ptr(field);
      if (value != nullptr && value->isString() && !value->getString().empty()) command.search[field] = value->getString();
    }
    if (!command.search.contains("utterance")) command.search["utterance"] = command.search["title"];
  }
  return command;
}

} // namespace

bool VoiceCommand::isTransport() const {
  return oneOf(kTransport, type);
}

folly::dynamic VoiceCommand::payload() const {
  if (type == "search") {
    folly::dynamic query = folly::dynamic::object();
    for (const auto &[name, value] : search) query[name] = value;
    return query;
  }
  folly::dynamic command = folly::dynamic::object("type", type);
  if (type == "skip") command["seconds"] = seconds;
  if (type == "seek") command["position"] = position;
  if (type == "subtitles") command["enabled"] = enabled;
  return command;
}

std::optional<VoiceCommand> parseVoiceCommand(const std::string &json) {
  try {
    return read(folly::parseJson(json));
  } catch (const std::exception &error) {
    log(LogLevel::Warning, std::string("Voice: dropped a command the shell sent: ") + error.what());
    return std::nullopt;
  }
}

} // namespace rntizen
