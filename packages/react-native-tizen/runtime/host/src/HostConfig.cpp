#include "HostConfig.h"

#include <folly/json.h>

#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace rntizen {

namespace {

constexpr const char *kConfigFile = "react-native-tizen.json";
constexpr std::size_t kMaxConfigBytes = 64 * 1024;

std::string readConfigText(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) throw HostConfigError("cannot open " + path);
  std::ostringstream contents;
  contents << file.rdbuf();
  auto text = contents.str();
  if (text.size() > kMaxConfigBytes) throw HostConfigError(path + " is larger than 64 KiB");
  return text;
}

std::string requireString(const folly::dynamic &config, const char *field) {
  auto value = config.get_ptr(field);
  if (value == nullptr || !value->isString() || value->getString().empty()) {
    throw HostConfigError(std::string(kConfigFile) + ": `" + field + "` must be a non-empty string");
  }
  return value->getString();
}

std::optional<std::string> optionalString(const folly::dynamic &config, const char *field) {
  auto value = config.get_ptr(field);
  if (value == nullptr || value->isNull()) return std::nullopt;
  if (!value->isString()) throw HostConfigError(std::string(kConfigFile) + ": `" + field + "` must be a string");
  return value->getString();
}

bool optionalBool(const folly::dynamic &config, const char *field) {
  auto value = config.get_ptr(field);
  if (value == nullptr || value->isNull()) return false;
  if (!value->isBool()) throw HostConfigError(std::string(kConfigFile) + ": `" + field + "` must be a boolean");
  return value->getBool();
}

std::optional<LogEndpoint> optionalLogEndpoint(const folly::dynamic &config) {
  auto value = config.get_ptr("log");
  if (value == nullptr || value->isNull()) return std::nullopt;
  auto port = value->isObject() ? value->get_ptr("port") : nullptr;
  if (port == nullptr || !port->isInt() || port->getInt() <= 0 ||
      port->getInt() > std::numeric_limits<std::uint16_t>::max()) {
    throw HostConfigError(std::string(kConfigFile) + ": `log.port` must be a TCP port");
  }
  return LogEndpoint{.host = requireString(*value, "host"), .port = static_cast<std::uint16_t>(port->getInt())};
}

std::optional<std::string> voiceSearchUrl(const folly::dynamic &config) {
  auto voice = config.get_ptr("voice");
  if (voice == nullptr || voice->isNull()) return std::nullopt;
  if (!voice->isObject()) throw HostConfigError(std::string(kConfigFile) + ": `voice` must be an object");
  return optionalString(*voice, "searchUrl");
}

bool isUhd(const folly::dynamic &config) {
  const auto resolution = optionalString(config, "resolution").value_or("1080p");
  if (resolution != "1080p" && resolution != "4k") {
    throw HostConfigError(std::string(kConfigFile) + ": `resolution` must be 1080p or 4k");
  }
  return resolution == "4k";
}

} // namespace

HostConfig loadHostConfig(const std::string &resourceDir) {
  const auto config = folly::parseJson(readConfigText(resourceDir + "/" + kConfigFile));
  if (!config.isObject()) throw HostConfigError(std::string(kConfigFile) + " must hold a JSON object");
  return HostConfig{
      .appKey = requireString(config, "appKey"),
      .packageId = optionalString(config, "packageId").value_or(""),
      .bundleFile = requireString(config, "bundleFile"),
      .bundleUrl = optionalString(config, "bundleUrl"),
      .logEndpoint = optionalLogEndpoint(config),
      .development = optionalBool(config, "development"),
      .resourceDir = resourceDir,
      .uhd = isUhd(config),
      .backgroundMedia = optionalBool(config, "backgroundMedia"),
      .voice = config.get_ptr("voice") != nullptr && config.get_ptr("voice")->isObject(),
      .voiceSearchUrl = voiceSearchUrl(config),
  };
}

} // namespace rntizen
