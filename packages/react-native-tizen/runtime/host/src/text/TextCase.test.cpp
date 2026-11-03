#include "TextCase.h"

#include <doctest/doctest.h>

namespace rntizen {

using facebook::react::TextTransform;

TEST_CASE("transformedText uppercases ASCII, Latin-1 and Latin Extended-A") {
  CHECK(transformedText("été, œuvre, ÿ, ı", TextTransform::Uppercase) == "ÉTÉ, ŒUVRE, Ÿ, I");
}

TEST_CASE("transformedText lowercases ASCII, Latin-1 and Latin Extended-A") {
  CHECK(transformedText("ÉCOLE Ÿ İ Ł", TextTransform::Lowercase) == "école ÿ i ł");
}

TEST_CASE("transformedText capitalizes the first letter after a space, a tab or a no-break space") {
  CHECK(transformedText("élan vital\tà bientôt", TextTransform::Capitalize) == "Élan Vital\tÀ Bientôt");
}

TEST_CASE("transformedText leaves other characters and other transforms alone") {
  CHECK(transformedText("straße αβ", TextTransform::Uppercase) == "STRAßE αβ");
  CHECK(transformedText("Mixed Case", TextTransform::None) == "Mixed Case");
}

TEST_CASE("transformedText replaces a malformed sequence and keeps the text after it") {
  CHECK(transformedText("a\xC3" "b", TextTransform::Uppercase) == "A\xEF\xBF\xBD" "B");
}

} // namespace rntizen
