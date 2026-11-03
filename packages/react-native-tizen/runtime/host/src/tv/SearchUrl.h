#pragma once

#include <map>
#include <string>

namespace rntizen {

/// Fills `{query}`, `{utterance}`, `{title}`, `{genre}`, `{cast}` and
/// `{contentType}` in `templateUrl` with the search's fields, percent-encoded;
/// `{query}` is the title, else the utterance. A field the search lacks is empty.
std::string fillSearchUrl(const std::string &templateUrl, const std::map<std::string, std::string> &fields);

} // namespace rntizen
