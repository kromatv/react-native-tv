#include "SpeechLocale.h"

#include <algorithm>
#include <cctype>

namespace rntizen::expo::speech {

namespace {

std::string normalized(std::string_view tag) {
  std::string text(tag);
  std::ranges::replace(text, '_', '-');
  std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return text;
}

std::string languageOf(std::string_view tag) {
  const auto normal = normalized(tag);
  return normal.substr(0, normal.find('-'));
}

} // namespace

std::string toBcp47(std::string_view sttLanguage) {
  std::string tag(sttLanguage);
  std::ranges::replace(tag, '_', '-');
  return tag;
}

std::optional<std::string> sttLanguageFor(
    std::string_view requested, const std::vector<std::string> &supported, std::string_view defaultLanguage) {
  if (requested.empty()) return std::string(defaultLanguage);
  const auto wanted = normalized(requested);
  const auto isWanted = [&](const auto &candidate) { return normalized(candidate) == wanted; };
  if (const auto exact = std::ranges::find_if(supported, isWanted); exact != supported.end()) return *exact;
  const auto language = languageOf(requested);
  if (!defaultLanguage.empty() && languageOf(defaultLanguage) == language) return std::string(defaultLanguage);
  const auto isSame = [&](const auto &candidate) { return languageOf(candidate) == language; };
  if (const auto same = std::ranges::find_if(supported, isSame); same != supported.end()) return *same;
  return std::nullopt;
}

} // namespace rntizen::expo::speech
