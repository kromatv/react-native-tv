#pragma once

#include <cstdint>
#include <optional>

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

/// HarfBuzz's opaque `hb_blob_t`, `hb_face_t` and `hb_subset_input_t`.
struct HbBlob;
struct HbFace;
struct HbSubsetInput;

/// The HarfBuzz calls that instance a variable font, resolved from
/// libharfbuzz.so.0 and libharfbuzz-subset.so.0 at run time: Tizen 9 ships both
/// (HarfBuzz 8.2.2) but they are not Tizen native API.
struct HarfBuzzSubset {
  HbBlob *(*blobCreateFromFile)(const char *path);
  void (*blobDestroy)(HbBlob *blob);
  const char *(*blobData)(HbBlob *blob, unsigned *length);
  HbFace *(*faceCreate)(HbBlob *blob, unsigned index);
  void (*faceDestroy)(HbFace *face);
  HbBlob *(*faceBlob)(HbFace *face);
  unsigned (*axisInfos)(HbFace *face, unsigned start, unsigned *count, HbAxisInfo *infos);
  HbSubsetInput *(*inputCreate)();
  void (*inputDestroy)(HbSubsetInput *input);
  void (*inputKeepEverything)(HbSubsetInput *input);
  int (*pinAxisLocation)(HbSubsetInput *input, HbFace *face, std::uint32_t tag, float value);
  int (*pinAxisToDefault)(HbSubsetInput *input, HbFace *face, std::uint32_t tag);
  HbFace *(*subset)(HbFace *face, const HbSubsetInput *input);
};

/// The calls from the libraries named, or nothing when either is missing or
/// lacks one of them.
std::optional<HarfBuzzSubset> loadHarfBuzzSubset(const char *coreLibrary, const char *subsetLibrary);

/// The calls, or null when either library or any symbol is missing. Resolved once.
const HarfBuzzSubset *harfBuzzSubset();

consteval std::uint32_t hbTag(char a, char b, char c, char d) {
  return (static_cast<std::uint32_t>(a) << 24) | (static_cast<std::uint32_t>(b) << 16) |
      (static_cast<std::uint32_t>(c) << 8) | static_cast<std::uint32_t>(d);
}

} // namespace rntizen
