#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace rntizen {

/// A TV feature that needs a newer Tizen than the package's floor.
enum class TvFeature { Voice, VoiceTitleSelection, SmartHubPreview };

/// Why a TV running `platformVersion` lacks `feature`, or nullopt when it has it.
/// An unreadable version is taken as new enough.
std::optional<std::string> missingOnVersion(TvFeature feature, std::string_view platformVersion);

} // namespace rntizen
