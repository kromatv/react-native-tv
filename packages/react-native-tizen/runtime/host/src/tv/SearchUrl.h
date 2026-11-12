#pragma once

#include <functional>
#include <map>
#include <string>
#include <string_view>

namespace rntizen {

/// Fills `{query}`, `{utterance}`, `{title}`, `{genre}`, `{cast}` and
/// `{contentType}` in `templateUrl` with the search's fields, percent-encoded;
/// `{query}` is the title, else the utterance. A field the search lacks is empty.
std::string fillSearchUrl(std::string_view templateUrl, const std::map<std::string, std::string, std::less<>> &fields);

} // namespace rntizen
