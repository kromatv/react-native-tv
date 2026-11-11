#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace rnwebos {

/// A TV feature that needs a newer webOS than the package's floor.
enum class TvFeature { Voice, VoiceTitleSelection, SmartHubPreview };

/// Why a TV running `platformVersion` lacks `feature`, or nullopt when it has it.
/// An unreadable version is taken as new enough.
std::optional<std::string> missingOnVersion(TvFeature feature, std::string_view platformVersion);

} // namespace rnwebos
