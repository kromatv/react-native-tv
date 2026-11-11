#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace rntizen::expo::speech {

/// `fr_FR`, as the STT engine names a language, in BCP 47: `fr-FR`.
std::string toBcp47(std::string_view sttLanguage);

/// The engine's language for `requested`, a BCP 47 tag: the same tag, else the
/// default when it speaks the requested language, else the first that does.
/// An empty request takes the default; nullopt when none speaks it.
std::optional<std::string> sttLanguageFor(
    std::string_view requested, const std::vector<std::string> &supported, std::string_view defaultLanguage);

} // namespace rntizen::expo::speech
