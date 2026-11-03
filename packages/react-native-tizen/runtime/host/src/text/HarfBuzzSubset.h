#pragma once

#include <cstdint>

namespace rntizen {

/// `hb_ot_var_axis_info_t`, whose layout is HarfBuzz's public ABI.
struct HbAxisInfo {
  unsigned axisIndex;
  std::uint32_t tag;
  unsigned nameId;
  int flags;
  float minValue;
  float defaultValue;
  float maxValue;
  unsigned reserved;
};

/// The HarfBuzz calls that instance a variable font, resolved from
/// libharfbuzz.so.0 and libharfbuzz-subset.so.0 at run time: Tizen 9 ships both
/// (HarfBuzz 8.2.2) but they are not Tizen native API. Pointers are opaque
/// HarfBuzz objects.
struct HarfBuzzSubset {
  void *(*blobCreateFromFile)(const char *path);
  void (*blobDestroy)(void *blob);
  const char *(*blobData)(void *blob, unsigned *length);
  void *(*faceCreate)(void *blob, unsigned index);
  void (*faceDestroy)(void *face);
  void *(*faceBlob)(void *face);
  unsigned (*axisInfos)(void *face, unsigned start, unsigned *count, HbAxisInfo *infos);
  void *(*inputCreate)();
  void (*inputDestroy)(void *input);
  void (*inputKeepEverything)(void *input);
  int (*pinAxisLocation)(void *input, void *face, std::uint32_t tag, float value);
  int (*pinAxisToDefault)(void *input, void *face, std::uint32_t tag);
  void *(*subset)(void *face, const void *input);
};

/// The calls, or null when either library or any symbol is missing. Resolved once.
const HarfBuzzSubset *harfBuzzSubset();

constexpr std::uint32_t hbTag(char a, char b, char c, char d) {
  return (static_cast<std::uint32_t>(a) << 24) | (static_cast<std::uint32_t>(b) << 16) |
      (static_cast<std::uint32_t>(c) << 8) | static_cast<std::uint32_t>(d);
}

} // namespace rntizen
