#pragma once

#include <filesystem>
#include <vector>

namespace rntizen {

/// Writes one static font per weight, 100 to 900 in steps of 100, that a
/// variable font's `wght` axis covers, into `directory` as
/// `<stem>-<weight>.ttf`, each with its `OS/2` weight class set. Evas draws a
/// variable font at its default instance and emboldens it for bold
/// (efl-rendering.md section 5), so a family registered as these files draws
/// every weight from its own outlines and advances. Files already there are
/// reused. Instancing is HarfBuzz's subsetter, loaded at run time; returns no
/// file for a font without a `wght` axis, or when HarfBuzz is missing or fails.
/// Any thread.
std::vector<std::filesystem::path> writeWeightInstances(
    const std::filesystem::path &font, const std::filesystem::path &directory);

} // namespace rntizen
