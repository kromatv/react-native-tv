#include "HarfBuzzSubset.h"

#include "../logging/Log.h"

#include <dlfcn.h>

#include <algorithm>
#include <array>
#include <bit>
#include <functional>
#include <optional>

namespace rntizen {

namespace {

void *openLibrary(const char *name) {
  void *library = dlopen(name, RTLD_LAZY | RTLD_LOCAL | RTLD_NOLOAD);
  return library != nullptr ? library : dlopen(name, RTLD_LAZY | RTLD_LOCAL);
}

template <typename Function>
bool bind(void *library, const char *name, Function &function) {
  function = std::bit_cast<Function>(dlsym(library, name));
  return function != nullptr;
}

bool bindAll(void *core, void *subset, HarfBuzzSubset &api) {
  const std::array bound{
      bind(core, "hb_blob_create_from_file_or_fail", api.blobCreateFromFile),
      bind(core, "hb_blob_destroy", api.blobDestroy),
      bind(core, "hb_blob_get_data", api.blobData),
      bind(core, "hb_face_create", api.faceCreate),
      bind(core, "hb_face_destroy", api.faceDestroy),
      bind(core, "hb_face_reference_blob", api.faceBlob),
      bind(core, "hb_ot_var_get_axis_infos", api.axisInfos),
      bind(subset, "hb_subset_input_create_or_fail", api.inputCreate),
      bind(subset, "hb_subset_input_destroy", api.inputDestroy),
      bind(subset, "hb_subset_input_keep_everything", api.inputKeepEverything),
      bind(subset, "hb_subset_input_pin_axis_location", api.pinAxisLocation),
      bind(subset, "hb_subset_input_pin_axis_to_default", api.pinAxisToDefault),
      bind(subset, "hb_subset_or_fail", api.subset),
  };
  return std::ranges::all_of(bound, std::identity{});
}

} // namespace

std::optional<HarfBuzzSubset> loadHarfBuzzSubset(const char *coreLibrary, const char *subsetLibrary) {
  void *core = openLibrary(coreLibrary);
  void *subset = openLibrary(subsetLibrary);
  if (HarfBuzzSubset api{}; core != nullptr && subset != nullptr && bindAll(core, subset, api)) return api;
  log(LogLevel::Warning, "HarfBuzz's subsetter is unavailable: variable fonts draw at their default instance");
  return std::nullopt;
}

const HarfBuzzSubset *harfBuzzSubset() {
  static const auto resolved = loadHarfBuzzSubset("libharfbuzz.so.0", "libharfbuzz-subset.so.0");
  return resolved.has_value() ? &*resolved : nullptr;
}

} // namespace rntizen
