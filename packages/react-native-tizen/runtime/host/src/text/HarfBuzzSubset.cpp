#include "HarfBuzzSubset.h"

#include "../logging/Log.h"

#include <dlfcn.h>

namespace rntizen {

namespace {

void *openLibrary(const char *name) {
  void *library = dlopen(name, RTLD_LAZY | RTLD_LOCAL | RTLD_NOLOAD);
  return library != nullptr ? library : dlopen(name, RTLD_LAZY | RTLD_LOCAL);
}

template <typename Function>
bool bind(void *library, const char *name, Function &function) {
  function = reinterpret_cast<Function>(dlsym(library, name));
  return function != nullptr;
}

bool resolve(HarfBuzzSubset &api) {
  void *core = openLibrary("libharfbuzz.so.0");
  void *subset = openLibrary("libharfbuzz-subset.so.0");
  if (core == nullptr || subset == nullptr) return false;
  return bind(core, "hb_blob_create_from_file_or_fail", api.blobCreateFromFile) &&
      bind(core, "hb_blob_destroy", api.blobDestroy) && bind(core, "hb_blob_get_data", api.blobData) &&
      bind(core, "hb_face_create", api.faceCreate) && bind(core, "hb_face_destroy", api.faceDestroy) &&
      bind(core, "hb_face_reference_blob", api.faceBlob) && bind(core, "hb_ot_var_get_axis_infos", api.axisInfos) &&
      bind(subset, "hb_subset_input_create_or_fail", api.inputCreate) &&
      bind(subset, "hb_subset_input_destroy", api.inputDestroy) &&
      bind(subset, "hb_subset_input_keep_everything", api.inputKeepEverything) &&
      bind(subset, "hb_subset_input_pin_axis_location", api.pinAxisLocation) &&
      bind(subset, "hb_subset_input_pin_axis_to_default", api.pinAxisToDefault) &&
      bind(subset, "hb_subset_or_fail", api.subset);
}

} // namespace

const HarfBuzzSubset *harfBuzzSubset() {
  static const HarfBuzzSubset *const resolved = [] {
    static HarfBuzzSubset api{};
    if (resolve(api)) return &api;
    log(LogLevel::Warning, "HarfBuzz's subsetter is unavailable: variable fonts draw at their default instance");
    return static_cast<HarfBuzzSubset *>(nullptr);
  }();
  return resolved;
}

} // namespace rntizen
