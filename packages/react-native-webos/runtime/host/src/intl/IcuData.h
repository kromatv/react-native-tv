#pragma once

#include <string>

namespace rnwebos {

/// Points ICU at the `icudt<major>l.dat` the app ships in `resourceDir`, the
/// data Hermes' Intl and Unicode functions read. Call it before the runtime
/// starts.
void useIcuData(const std::string &resourceDir);

/// Makes the TV's language, a BCP 47 tag such as `en-GB`, ICU's default
/// locale, or the first locale the app ships when it ships none for that
/// language. Call it before the runtime starts.
void useDeviceLocale(const std::string &languageTag);

/// ICU's default locale as a BCP 47 tag of its language and region, such as `en-US`.
std::string defaultLanguageTag();

} // namespace rnwebos
