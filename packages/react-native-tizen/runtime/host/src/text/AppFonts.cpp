#include "AppFonts.h"

#include "SfntTables.h"

#include <cmath>
#include <cstdlib>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <mutex>
#include <system_error>
#include <tuple>

namespace rntizen {

namespace fs = std::filesystem;

namespace {

struct AppFontFile {
  fs::path path;
  SfntStyle style;
  int unitsPerEm;
};

struct Registry {
  std::mutex mutex;
  std::map<std::string, std::vector<AppFontFile>, std::less<>> families;
  std::map<std::tuple<std::string, int, bool, int>, std::optional<fs::path>> tracked;
};

Registry &registry() {
  static Registry instance;
  return instance;
}

std::string contentsOf(const fs::path &file) {
  std::ifstream in(file, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

const AppFontFile *closest(const std::vector<AppFontFile> &files, int weight, bool italic) {
  const AppFontFile *best = nullptr;
  auto distance = [&](const AppFontFile &file) {
    return std::abs(file.style.weight - weight) * 2 + (file.style.weight < weight ? 1 : 0) +
        (file.style.italic == italic ? 0 : 10000);
  };
  for (const auto &file : files) {
    if (best == nullptr || distance(file) < distance(*best)) best = &file;
  }
  return best;
}

std::optional<fs::path> writeTracked(const fs::path &font, int units) {
  const auto directory = font.parent_path().parent_path() / "tracked";
  const auto target = directory /
      std::format(
          "{}-{}-{}{}", font.parent_path().filename().string(), font.stem().string(), units, font.extension().string());
  std::error_code error;
  if (fs::file_size(target, error) > 0 && !error) return target;
  auto bytes = contentsOf(font);
  if (!trackSfntAdvances(bytes, units)) return std::nullopt;
  fs::create_directories(directory, error);
  auto partial = target;
  partial += ".partial";
  {
    std::ofstream out(partial, std::ios::binary | std::ios::trunc);
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (!out.good()) return std::nullopt;
  }
  fs::rename(partial, target, error);
  return error ? std::nullopt : std::optional(target);
}

} // namespace

void registerAppFontFiles(const std::string &family, const std::vector<fs::path> &files) {
  std::vector<AppFontFile> described;
  for (const auto &file : files) {
    const auto bytes = contentsOf(file);
    const auto style = sfntStyle(bytes);
    const auto unitsPerEm = sfntUnitsPerEm(bytes);
    if (style.has_value() && unitsPerEm.has_value()) described.emplace_back(file, *style, *unitsPerEm);
  }
  auto &state = registry();
  std::lock_guard lock(state.mutex);
  state.families[family] = std::move(described);
  std::erase_if(state.tracked, [&family](const auto &entry) { return std::get<0>(entry.first) == family; });
}

std::optional<fs::path> trackedAppFont(const std::string &family, int weight, bool italic, double em) {
  auto &state = registry();
  std::lock_guard lock(state.mutex);
  const auto found = state.families.find(family);
  if (found == state.families.end()) return std::nullopt;
  const auto *file = closest(found->second, weight, italic);
  if (file == nullptr) return std::nullopt;
  const auto units = static_cast<int>(std::lround(em * file->unitsPerEm));
  if (units == 0) return std::nullopt;
  const auto key = std::make_tuple(family, file->style.weight, file->style.italic, units);
  if (const auto cached = state.tracked.find(key); cached != state.tracked.end()) return cached->second;
  return state.tracked[key] = writeTracked(file->path, units);
}

} // namespace rntizen
