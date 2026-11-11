#include "FontInstances.h"

#include "../logging/Log.h"
#include "HarfBuzzSubset.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>
#include <fstream>
#include <optional>
#include <string>
#include <system_error>

namespace rntizen {

namespace fs = std::filesystem;

namespace {

constexpr std::uint32_t kWeightAxis = hbTag('w', 'g', 'h', 't');

struct Face {
  const HarfBuzzSubset &hb;
  HbBlob *const blob;
  HbFace *const face;

  Face(const HarfBuzzSubset &api, const fs::path &file)
      : hb(api), blob(api.blobCreateFromFile(file.c_str())), face(blob != nullptr ? api.faceCreate(blob, 0) : nullptr) {}
  ~Face() {
    if (face != nullptr) hb.faceDestroy(face);
    if (blob != nullptr) hb.blobDestroy(blob);
  }
  Face(const Face &) = delete;
  Face &operator=(const Face &) = delete;
};

std::vector<HbAxisInfo> axesOf(const Face &font) {
  unsigned count = font.hb.axisInfos(font.face, 0, nullptr, nullptr);
  std::vector<HbAxisInfo> axes(count);
  if (count > 0) font.hb.axisInfos(font.face, 0, &count, axes.data());
  axes.resize(count);
  return axes;
}

std::optional<HbAxisInfo> weightAxis(const std::vector<HbAxisInfo> &axes) {
  const auto found = std::ranges::find_if(axes, [](const auto &axis) { return axis.tag == kWeightAxis; });
  return found == axes.end() ? std::nullopt : std::optional(*found);
}

std::vector<int> weightsWithin(const HbAxisInfo &axis) {
  std::vector<int> weights;
  for (int weight = 100; weight <= 900; weight += 100) {
    if (weight >= std::ceil(axis.minValue) && weight <= std::floor(axis.maxValue)) weights.push_back(weight);
  }
  return weights;
}

bool writeInstance(const Face &font, const std::vector<HbAxisInfo> &axes, int weight, const fs::path &target) {
  const auto &hb = font.hb;
  auto *input = hb.inputCreate();
  if (input == nullptr) return false;
  hb.inputKeepEverything(input);
  for (const auto &axis : axes) {
    if (axis.tag == kWeightAxis) {
      hb.pinAxisLocation(input, font.face, axis.tag, static_cast<float>(weight));
    } else {
      hb.pinAxisToDefault(input, font.face, axis.tag);
    }
  }
  auto *instance = hb.subset(font.face, input);
  hb.inputDestroy(input);
  if (instance == nullptr) return false;
  auto *blob = hb.faceBlob(instance);
  unsigned length = 0;
  const char *data = hb.blobData(blob, &length);
  auto partial = target;
  partial += ".partial";
  bool written = false;
  {
    std::ofstream out(partial, std::ios::binary | std::ios::trunc);
    out.write(data, length);
    written = length > 0 && out.good();
  }
  hb.blobDestroy(blob);
  hb.faceDestroy(instance);
  std::error_code error;
  if (written) fs::rename(partial, target, error);
  if (!written || error) fs::remove(partial, error);
  return written && !error;
}

bool hasInstance(const fs::path &target) {
  std::error_code error;
  return fs::file_size(target, error) > 0 && !error;
}

void removeFiles(const std::vector<fs::path> &files) {
  std::error_code error;
  for (const auto &file : files) fs::remove(file, error);
}

} // namespace

std::vector<fs::path> writeWeightInstances(const fs::path &font, const fs::path &directory) {
  const auto *hb = harfBuzzSubset();
  if (hb == nullptr) return {};
  return writeWeightInstances(*hb, font, directory);
}

std::vector<fs::path> writeWeightInstances(const HarfBuzzSubset &hb, const fs::path &font, const fs::path &directory) {
  const Face source(hb, font);
  if (source.face == nullptr) return {};
  const auto axes = axesOf(source);
  const auto weight = weightAxis(axes);
  if (!weight) return {};
  const auto started = std::chrono::steady_clock::now();
  std::vector<fs::path> files;
  int written = 0;
  for (const int value : weightsWithin(*weight)) {
    const auto target = directory / std::format("{}-{}.ttf", font.stem().string(), value);
    if (!hasInstance(target)) {
      if (!writeInstance(source, axes, value, target)) {
        log(LogLevel::Warning, std::format("HarfBuzz could not instance {} at weight {}", font.filename().string(), value));
        removeFiles(files);
        return {};
      }
      ++written;
    }
    files.push_back(target);
  }
  if (written > 0) {
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started);
    log(LogLevel::Info,
        std::format("Wrote {} weights of {} in {} ms", written, font.filename().string(), elapsed.count()));
  }
  return files;
}

} // namespace rntizen
