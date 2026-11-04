#include "TvFeatures.h"

#include "../modules/TizenVersion.h"

#include <array>
#include <cstddef>
#include <format>

namespace rntizen {

namespace {

struct Requirement {
  std::string_view name;
  TizenVersion since;
};

// In TvFeature's order. Voice: Samsung's VoiceInteraction page, title selection
// "Since 6.5" (tizen-tv-app-integration.md). Smart Hub Preview: "supported since
// 2016 TV models" (tizen-smart-hub-preview.md).
constexpr std::array<Requirement, 3> kRequirements{{
    {.name = "Voice control", .since = {.major = 6, .minor = 0}},
    {.name = "Selecting a title by voice", .since = {.major = 6, .minor = 5}},
    {.name = "Smart Hub Preview", .since = {.major = 2, .minor = 4}},
}};

} // namespace

std::optional<std::string> missingOnVersion(TvFeature feature, std::string_view platformVersion) {
  const auto version = parseTizenVersion(platformVersion);
  const auto &requirement = kRequirements.at(static_cast<std::size_t>(feature));
  if (!version || *version >= requirement.since) return std::nullopt;
  return std::format(
      "{} needs Tizen {}.{}, and this TV runs {}",
      requirement.name,
      requirement.since.major,
      requirement.since.minor,
      platformVersion);
}

} // namespace rntizen
