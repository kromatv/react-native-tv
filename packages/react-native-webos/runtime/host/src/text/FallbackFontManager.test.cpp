#include "FallbackFontManager.h"

#include "../render/OverlayText.h"

#include <doctest/doctest.h>

#include "include/core/SkFontStyle.h"
#include "include/core/SkTypeface.h"
#include "include/ports/SkFontMgr_directory.h"

namespace rnwebos {

namespace {

constexpr const char *kFontDirectory = "/usr/share/fonts";
constexpr const char *kLatinFamily = "DejaVu Sans";
constexpr SkUnichar kDevanagariA = 0x0905;
constexpr SkUnichar kUnassigned = 0x0378;

sk_sp<SkFontMgr> directoryFonts() {
  return SkFontMgr_New_Custom_Directory(kFontDirectory);
}

SkString familyOf(const sk_sp<SkTypeface> &typeface) {
  SkString name;
  typeface->getFamilyName(&name);
  return name;
}

} // namespace

TEST_CASE("the directory font manager alone has no font for a character a family lacks") {
  const auto fonts = directoryFonts();

  CHECK(fonts->matchFamilyStyle(kLatinFamily, SkFontStyle::Normal())->unicharToGlyph(kDevanagariA) == 0);
  CHECK(fonts->matchFamilyStyleCharacter(kLatinFamily, SkFontStyle::Normal(), nullptr, 0, kDevanagariA) == nullptr);
}

TEST_CASE("withCharacterFallback answers the first family that has the character, at the style asked for") {
  const auto fonts = withCharacterFallback(directoryFonts());

  const auto regular = fonts->matchFamilyStyleCharacter(kLatinFamily, SkFontStyle::Normal(), nullptr, 0, kDevanagariA);
  const auto bold = fonts->matchFamilyStyleCharacter(kLatinFamily, SkFontStyle::Bold(), nullptr, 0, kDevanagariA);

  REQUIRE(regular != nullptr);
  REQUIRE(bold != nullptr);
  CHECK(regular->unicharToGlyph(kDevanagariA) != 0);
  CHECK(bold->unicharToGlyph(kDevanagariA) != 0);
  CHECK(familyOf(regular) != SkString(kLatinFamily));
  CHECK(familyOf(bold) == familyOf(regular));
  CHECK(bold->fontStyle().weight() > regular->fontStyle().weight());
}

TEST_CASE("withCharacterFallback answers nothing for a character no family has, however often it is asked") {
  const auto fonts = withCharacterFallback(directoryFonts());

  CHECK(fonts->matchFamilyStyleCharacter(kLatinFamily, SkFontStyle::Normal(), nullptr, 0, kUnassigned) == nullptr);
  CHECK(fonts->matchFamilyStyleCharacter(kLatinFamily, SkFontStyle::Normal(), nullptr, 0, kUnassigned) == nullptr);
}

TEST_CASE("withCharacterFallback lists and matches the families of the manager it wraps") {
  const auto plain = directoryFonts();
  const auto fonts = withCharacterFallback(plain);
  SkString first;
  SkString firstPlain;
  fonts->getFamilyName(0, &first);
  plain->getFamilyName(0, &firstPlain);

  CHECK(fonts->countFamilies() == plain->countFamilies());
  CHECK(fonts->countFamilies() > 2);
  CHECK(first == firstPlain);
  CHECK(fonts->createStyleSet(0)->count() == plain->createStyleSet(0)->count());
  CHECK(fonts->matchFamily(kLatinFamily)->count() > 0);
  CHECK(familyOf(fonts->matchFamilyStyle(kLatinFamily, SkFontStyle::Bold())) == SkString(kLatinFamily));
  CHECK(fonts->matchFamily("No Such Family")->count() == 0);
}

TEST_CASE("withCharacterFallback reads a font file as the manager it wraps does") {
  const auto fonts = withCharacterFallback(directoryFonts());

  const auto typeface = fonts->makeFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 0);

  REQUIRE(typeface != nullptr);
  CHECK(familyOf(typeface) == SkString(kLatinFamily));
  CHECK(fonts->makeFromFile("/nonexistent/font.ttf", 0) == nullptr);
}

TEST_CASE("the text engine draws a character its family lacks with another family's glyph") {
  const auto paragraph = overlayParagraph({{.text = "A \xE0\xA4\x85"}}, 400);
  const auto missing = overlayParagraph({{.text = "A \xCD\xB8"}}, 400);

  CHECK(paragraph->unresolvedCodepoints().count(kDevanagariA) == 0);
  CHECK(missing->unresolvedCodepoints().count(kUnassigned) == 1);
}

} // namespace rnwebos
