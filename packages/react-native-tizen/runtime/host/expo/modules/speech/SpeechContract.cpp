#include "SpeechContract.h"

#include <algorithm>

namespace rntizen::expo::speech {

SpeechOptions speechOptionsFrom(const folly::dynamic &options) {
  SpeechOptions parsed;
  if (!options.isObject()) return parsed;
  if (const auto *lang = options.get_ptr("lang"); lang != nullptr && lang->isString()) parsed.language = lang->getString();
  if (const auto *interim = options.get_ptr("interimResults"); interim != nullptr && interim->isBool()) {
    parsed.interimResults = interim->getBool();
  }
  if (const auto *max = options.get_ptr("maxAlternatives"); max != nullptr && max->isNumber()) {
    parsed.maxAlternatives = std::max(1, static_cast<int>(max->asDouble()));
  }
  return parsed;
}

const char *speechStateName(SpeechState state) {
  using enum SpeechState;
  switch (state) {
    case Starting:
      return "starting";
    case Recognizing:
      return "recognizing";
    case Stopping:
      return "stopping";
    case Inactive:
      break;
  }
  return "inactive";
}

} // namespace rntizen::expo::speech
