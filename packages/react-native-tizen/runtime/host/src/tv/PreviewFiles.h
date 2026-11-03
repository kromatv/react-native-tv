#pragma once

#include <optional>
#include <string>

namespace rntizen {

/// The preview Samsung last took, kept in the package's data directory, which
/// the preview service app reads too.
void writePreviewFile(const std::string &samsungJson);

/// Where the preview service fetches tiles while the app is closed; nullopt
/// removes it.
void writePreviewSource(const std::optional<std::string> &sourceJson);

} // namespace rntizen
