#include "TextblockMarkup.h"

#include "TextCase.h"
#include "TextblockFormat.h"

#include <Evas.h>

#include <cstdlib>

namespace rntizen {

namespace {

std::string escaped(const std::string &text) {
  char *markup = evas_textblock_text_utf8_to_markup(nullptr, text.c_str());
  if (markup == nullptr) return {};
  std::string result(markup);
  std::free(markup);
  return result;
}

} // namespace

TextblockContent textblockContent(
    const facebook::react::AttributedString &attributedString,
    const facebook::react::ParagraphAttributes &paragraphAttributes,
    facebook::react::Float scale) {
  const auto &fragments = attributedString.getFragments();
  const auto alignment = fragments.empty() ? std::nullopt : fragments.front().textAttributes.alignment;
  TextblockContent content;
  content.style = "DEFAULT='" + paragraphFormat(paragraphAttributes, alignment, scale) + "'";
  for (const auto &fragment : fragments) {
    if (fragment.isAttachment()) continue;
    const auto &attributes = fragment.textAttributes;
    const auto text = attributes.textTransform ? transformedText(fragment.string, *attributes.textTransform) : fragment.string;
    const auto run = fragmentFormat(attributes, scale);
    content.markup += "<" + run.format + ">" + escaped(text) + "</>";
    if (!content.tightLineShift.has_value()) content.tightLineShift = run.tightLineShift;
  }
  return content;
}

} // namespace rntizen
