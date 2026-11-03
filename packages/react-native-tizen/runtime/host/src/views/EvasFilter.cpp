#include "EvasFilter.h"

#include "../logging/Log.h"

#include <dlfcn.h>

#include <string>

namespace rntizen {

namespace {

using ProgramSet = void (*)(Eo *, const char *, const char *);
using DataSet = void (*)(Eo *, const char *, const char *, Eina_Bool);

struct FilterApi {
  ProgramSet programSet{nullptr};
  DataSet dataSet{nullptr};
};

FilterApi resolve() {
  void *library = dlopen("libefl.so.1", RTLD_LAZY | RTLD_LOCAL | RTLD_NOLOAD);
  if (library == nullptr) library = dlopen("libefl.so.1", RTLD_LAZY | RTLD_LOCAL);
  if (library == nullptr) return {};
  return {
      .programSet = reinterpret_cast<ProgramSet>(dlsym(library, "efl_gfx_filter_program_set")),
      .dataSet = reinterpret_cast<DataSet>(dlsym(library, "efl_gfx_filter_data_set")),
  };
}

const FilterApi &api() {
  static const FilterApi resolved = [] {
    auto found = resolve();
    if (found.programSet == nullptr || found.dataSet == nullptr) {
      log(LogLevel::Warning, "Evas filters unavailable: blur views draw their tint only");
      return FilterApi{};
    }
    log(LogLevel::Info, "Evas filters resolved from libefl.so.1");
    return found;
  }();
  return resolved;
}

} // namespace

bool evasFiltersAvailable() {
  return api().programSet != nullptr;
}

void setEvasFilterProgram(Evas_Object *object, const char *code) {
  if (evasFiltersAvailable()) api().programSet(object, code, nullptr);
}

void setEvasFilterNumber(Evas_Object *object, std::string_view name, int value) {
  if (!evasFiltersAvailable()) return;
  const std::string key(name);
  const auto expression = "tonumber(" + std::to_string(value) + ")";
  api().dataSet(object, key.c_str(), expression.c_str(), EINA_TRUE);
}

} // namespace rntizen
