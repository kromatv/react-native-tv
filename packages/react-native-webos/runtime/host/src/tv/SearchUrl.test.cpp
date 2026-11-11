#include "SearchUrl.h"

#include <doctest/doctest.h>

namespace rntizen {

TEST_CASE("fillSearchUrl fills each placeholder with its field, percent-encoded") {
  const std::map<std::string, std::string, std::less<>> fields{
      {"title", "Dune: Part Two"}, {"genre", "sci-fi"}, {"cast", "Zendaya"}};

  const auto url = fillSearchUrl("https://example.tv/search?q={title}&g={genre}&c={cast}", fields);

  CHECK(url == "https://example.tv/search?q=Dune%3A%20Part%20Two&g=sci-fi&c=Zendaya");
}

TEST_CASE("fillSearchUrl's query is the title, else the utterance") {
  const std::map<std::string, std::string, std::less<>> titled{{"title", "Dune"}, {"utterance", "find dune"}};
  const std::map<std::string, std::string, std::less<>> spoken{{"utterance", "find dune"}};

  CHECK(fillSearchUrl("/s?q={query}", titled) == "/s?q=Dune");
  CHECK(fillSearchUrl("/s?q={query}", spoken) == "/s?q=find%20dune");
}

TEST_CASE("fillSearchUrl encodes every byte of a non-ASCII field") {
  const std::map<std::string, std::string, std::less<>> fields{{"title", "Am\xC3\xA9lie"}};

  CHECK(fillSearchUrl("{title}", fields) == "Am%C3%A9lie");
}

TEST_CASE("fillSearchUrl empties a missing field and keeps an unclosed brace") {
  const std::map<std::string, std::string, std::less<>> fields;

  CHECK(fillSearchUrl("/s?g={genre}&x={open", fields) == "/s?g=&x={open");
  CHECK(fillSearchUrl("/plain", fields) == "/plain");
}

} // namespace rntizen
