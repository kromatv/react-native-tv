#include "LaunchParameters.h"

#include <folly/json.h>

#include <array>

namespace rnwebos {

namespace {

constexpr std::size_t kMaxArgumentBytes = 64 * 1024;
constexpr std::size_t kMaxDeepLinkBytes = 8 * 1024;

// What the app manager adds beside the caller's own parameters.
constexpr std::array<std::string_view, 3> kProcessKeys{"nid", "@system_native_app", "preload"};
constexpr std::array<std::string_view, 3> kLaunchEventKeys{"event", "reason", "appId"};

std::string stringAt(const folly::dynamic &object, const char *key) {
  const auto *value = object.get_ptr(key);
  return value != nullptr && value->isString() ? value->getString() : "";
}

bool isLaunchEvent(const folly::dynamic &argument) {
  return stringAt(argument, "event") == "launch";
}

folly::dynamic withoutKeys(folly::dynamic object, const auto &keys) {
  for (const auto key : keys) object.erase(key);
  return object;
}

folly::dynamic callerParameters(const folly::dynamic &argument) {
  const auto *nested = argument.get_ptr("parameters");
  if (isLaunchEvent(argument) && nested != nullptr && nested->isObject()) return *nested;
  auto own = withoutKeys(argument, kProcessKeys);
  return isLaunchEvent(argument) ? withoutKeys(std::move(own), kLaunchEventKeys) : own;
}

std::optional<int> interfaceVersionOf(const folly::dynamic &argument) {
  const auto *version = argument.get_ptr("interfaceVersion");
  if (version != nullptr && version->isInt()) return static_cast<int>(version->getInt());
  return std::nullopt;
}

} // namespace

LaunchParameters parseLaunchArgument(std::string_view argument) {
  if (argument.empty() || argument.size() > kMaxArgumentBytes || argument.front() != '{') return {};
  folly::dynamic parsed;
  try {
    parsed = folly::parseJson(argument);
  } catch (const std::exception &) {
    return {};
  }
  if (!parsed.isObject()) return {};
  return {
      .parameters = callerParameters(parsed),
      .reason = stringAt(parsed, "reason"),
      .interfaceVersion = interfaceVersionOf(parsed),
  };
}

LaunchParameters launchParametersOf(int argc, char **argv) {
  if (argc < 2 || argv[argc - 1] == nullptr) return {};
  return parseLaunchArgument(argv[argc - 1]);
}

std::optional<std::string> deepLinkOf(const folly::dynamic &parameters) {
  if (!parameters.isObject()) return std::nullopt;
  auto target = stringAt(parameters, "contentTarget");
  if (target.empty() || target.size() > kMaxDeepLinkBytes) return std::nullopt;
  return target;
}

} // namespace rnwebos
