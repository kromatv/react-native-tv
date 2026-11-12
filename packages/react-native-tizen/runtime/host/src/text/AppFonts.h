#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace rntizen {

/// Remembers the files an app registered for `family` (expo-font's name for
/// it), each with the weight and slant fontconfig matches it on. Any thread.
void registerAppFontFiles(const std::string &family, const std::vector<std::filesystem::path> &files);

/// The app's font for `family` at `weight` (100 to 900) and `italic`, with
/// `em` (letter spacing over font size) added after every glyph: a copy of the
/// file fontconfig would pick, its advance widths widened, written once beside
/// the registered fonts. Evas loads a font by its path, so text set in it is
/// measured, wrapped and drawn with the spacing (efl-rendering.md section 5).
/// Nothing for a family the app did not register, or a spacing under one font
/// unit. Main thread or any thread.
std::optional<std::filesystem::path> trackedAppFont(
    const std::string &family, int weight, bool italic, double em);

} // namespace rntizen
