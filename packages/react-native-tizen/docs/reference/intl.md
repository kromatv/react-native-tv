# Intl and ICU

What Hermes, ICU, FormatJS and Tizen provide for `Intl`, locale-aware strings
and Unicode text, and what this package builds from them. Read it before
touching `runtime/cmake/Icu.cmake`, `runtime/cmake/Hermes.cmake`,
`runtime/host/src/intl/`, `src/runtime/icu-*.ts`, `src/metro/intl-locale-data.ts`
or `react-native-overrides/intl/`.

Tags: **D** documented, **S** read in source, **I** inferred (reason given),
**O** observed on the build machine or in the simulator (when), **H** verified
on a TV. Hermes is `hermes-v260318099.0.4`, ICU 78.3, FormatJS as pinned in
`package.json`; checked on 2026-11-03.

## Hermes

| Fact | Tag | Source |
|---|---|---|
| Built with `HERMES_UNICODE_LITE`, Hermes compares strings by UTF-16 code unit, leaves non-ASCII case conversion and `normalize()` untouched, and `Date.prototype.toLocaleString` returns the text "dateFormat not implemented". `toLocaleUpperCase` always takes the platform path, so with the lite tables it changes nothing, ASCII included. | S | `lib/Platform/Unicode/PlatformUnicodeLite.cpp:15-50`, `lib/VM/JSLib/String.cpp:935-1015` |
| `HERMES_ENABLE_INTL` adds `Intl.Collator`, `Intl.DateTimeFormat`, `Intl.NumberFormat`, `Intl.getCanonicalLocales` and the `toLocale*` methods; Hermes has no `PluralRules`, `RelativeTimeFormat`, `ListFormat`, `DisplayNames`, `Segmenter` or `Locale` on any platform. | S | `doc/IntlAPIs.md`, "Supported on both platforms"; no other constructor in `lib/` or `include/` |
| Off Android and Apple, Intl is `PlatformIntlICU.cpp` on ICU 65 or later. Its `Collator` and `DateTimeFormat.format` are ICU's; its `NumberFormat` is a stub that formats with `std::to_string` and resolves every locale to `en-US`, and `formatToParts` of both returns one `integer` part. | S | `lib/Platform/Intl/CMakeLists.txt:24-38`, `PlatformIntlICU.cpp:966-1028`, `CMakeLists.txt:637-642` |
| Hermes links ICU to the static `hermesPlatformIntl` archive only, not to the object library it compiles from, and adds ICU's include directories only when it searches for ICU itself. | S | `lib/Platform/Intl/CMakeLists.txt:37`, `CMakeLists.txt:630-659` |
| Hermes' `DateTimeFormat` does not check `udat_open`: when ICU cannot open a formatter, the next `format` dereferences null. | O | simulator, 2026-11-03: SIGSEGV in `icu_78::DateFormat::format (this=0x0)` from `PlatformIntlICU.cpp:948`, with ICU data missing a tree |

## ICU

| Fact | Tag | Source |
|---|---|---|
| ICU names its common data file `icudt<major><l\|b\|e>.dat`, little-endian `l` on ARM, and looks for it in the directory `u_setDataDirectory` set, else `$ICU_DATA`, else the compiled-in default. | D, S | [icu-data], "ICU Data Directory"; `common/unicode/utypes.h:130-154`, `common/putil.cpp:1316,1426-1495` |
| "`u_setDataDirectory()` is not thread-safe. Call it _before_ calling ICU APIs from multiple threads." | D | [icu-data], "ICU Data Directory" |
| "Only plain C APIs remain compatible across ICU releases." "Stable C++ APIs are _source_ compatible, but applications using them must be recompiled when moving between ICU releases." | D | [icu-design], "ICU Binary Compatibility" |
| By default ICU renames every C function and C++ class with its version (`ucnv_open_3_4`); `U_DISABLE_RENAMING` turns that off, in ICU and in the caller alike. | D | [icu-design], "Linking against multiple versions of ICU4C" |
| Hermes' Intl calls ICU's C++ API (`icu::TimeZone`, `icu::UnicodeString`, `icu::StringEnumeration`), not only its C API. | S | `PlatformIntlICU.cpp:25-31,44-54,108-115` |
| ICU's default locale comes from `LC_ALL`, then `LC_MESSAGES`, then `LANG`; `C` and `POSIX` become `en_US_POSIX`. | S | `common/putil.cpp:1594-1640` |
| ICU's default time zone comes from `TZ`, then the target of the `/etc/localtime` link under a `zoneinfo/` directory, then a search of the zoneinfo files for one equal to `/etc/localtime`. | S | `common/putil.cpp:687-760,1026` |
| A common data file is a header, a table of contents (count, then name and data offsets) and the items, each aligned on 16 bytes; a tree's `res_index.res` lists the locales `uloc_getAvailable` reports. Hermes resolves locales against that list. | S | `common/ucmndata.h`, `common/uresdata.h`; Hermes `impl_icu/LocaleResolver.cpp:28-40` |
| `DateTimePatternGenerator` fails with `U_MISSING_RESOURCE_ERROR` without the `curr` tree of the locale: the number symbols a date pattern uses read it. Without `lang`, `region`, `unit`, `rbnf`, `brkitr`, `translit` and the converters, collation, date formatting and case mapping work. | O | simulator, 2026-11-03: `udatpg_open("fr")` on trimmed data, one dropped tree added back at a time |

## FormatJS

| Fact | Tag | Source |
|---|---|---|
| `polyfill-force.js` of `@formatjs/intl-pluralrules`, `intl-numberformat` and `intl-relativetimeformat` replaces the service whatever the engine has; the `NumberFormat` one also replaces `Number.prototype.toLocaleString`. They need `Intl.getCanonicalLocales` and each other, in that order. | S | each package's `polyfill-force.js`; `intl-numberformat/polyfill-force.js:3085-3096` |
| Each service's default locale is the first locale whose data loaded, kept in a static `__defaultLocale`. | S | `intl-pluralrules/polyfill-force.js:1292-1305`, `intl-numberformat/polyfill-force.js:3040-3054`, `intl-relativetimeformat/polyfill-force.js:668-681` |
| FormatJS has no `Intl.Collator`. Its `DateTimeFormat` formats in UTC unless told a time zone, and needs FormatJS's own time zone data for any other (`add-golden-tz.js`, 803 KB). | S | `@formatjs/intl-datetimeformat@8.0.2/index.js:2528-2558` |

## Tizen

| Fact | Tag | Source |
|---|---|---|
| Tizen 9.0 ships `libicuuc.so.74`, Tizen 10.0 `libicuuc.so.76`. | O | rootstraps, `usr/lib/libicuuc.so.*` |
| Samsung builds ICU without symbol renaming: `u_strToUpper` and `icu::TimeZone::createDefault()` carry no version, so the two releases differ by soname and ABI, not by symbol name. | O | `llvm-nm -D` on both rootstraps' `libicuuc.so` and `libicui18n.so`, 2026-11-03 |
| Loading whichever `libicuuc.so.*` a TV has would serve a C caller across both years, but not Hermes, whose C++ calls ICU requires to be recompiled per release. | I | the two rows above and [icu-design] |
| Which `LANG` and `TZ` a TV gives an app process, and whether `/etc/localtime` names the zone, is not documented. | I | not found in Samsung's or Tizen's docs, 2026-11-03 |
| flutter-tizen ships ICU data in the package's `res/`. | D | [ft-package](https://github.com/flutter-tizen/flutter-tizen/blob/master/lib/build_targets/package.dart) |

## What this means

- The runtime compiles ICU 78.3 in, symbols hidden, and builds Hermes with its
  Intl on it (`docs/decisions.md`, "ICU inside the runtime"). The TV's ICU is not
  linked: a package built against one soname does not load on the other year.
- Hermes' `NumberFormat` stub and missing services are replaced by FormatJS,
  in every Tizen bundle, with the app's locales; the device's locale, as
  Hermes' ICU resolves it, becomes their default.
- A trimmed data file must keep the `curr` tree with the main, `coll` and `zone`
  trees of each language, or Hermes crashes on the first date it formats.
- Until a TV's `LANG` and time zone are verified, `new Intl.DateTimeFormat()`
  without a locale and every local time may differ between TVs.

[icu-data]: https://unicode-org.github.io/icu/userguide/icu_data/
[icu-design]: https://unicode-org.github.io/icu/userguide/icu/design.html
