#include "FontInstances.h"

#include <doctest/doctest.h>

#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <vector>

namespace rntizen {

namespace {

namespace fs = std::filesystem;

constexpr std::uint32_t kWidthAxis = hbTag('w', 'd', 't', 'h');

struct FakeHarfBuzz {
  std::array<char, 3> objects{};
  bool hasWeightAxis{true};
  bool inputFails{false};
  int failingSubset{0};
  int subsets{0};
  int pinned{0};
  int defaulted{0};
};

FakeHarfBuzz fake;

HbBlob *blobCreateFromFile(const char *path) {
  return fs::exists(path) ? std::bit_cast<HbBlob *>(&fake.objects[0]) : nullptr;
}

void blobDestroy(HbBlob *) {}

const char *blobData(HbBlob *, unsigned *length) {
  *length = 4;
  return "font";
}

HbFace *faceCreate(HbBlob *, unsigned) {
  return std::bit_cast<HbFace *>(&fake.objects[1]);
}

void faceDestroy(HbFace *) {}

HbBlob *faceBlob(HbFace *) {
  return std::bit_cast<HbBlob *>(&fake.objects[0]);
}

unsigned axisInfos(HbFace *, unsigned, unsigned *count, HbAxisInfo *infos) {
  const std::array axes{
      HbAxisInfo{.axisIndex = 0, .tag = fake.hasWeightAxis ? hbTag('w', 'g', 'h', 't') : hbTag('s', 'l', 'n', 't'),
                 .nameId = 0, .flags = 0, .minValue = 250, .defaultValue = 400, .maxValue = 720, .reserved = 0},
      HbAxisInfo{.axisIndex = 1, .tag = kWidthAxis, .nameId = 0, .flags = 0, .minValue = 75, .defaultValue = 100,
                 .maxValue = 100, .reserved = 0}};
  if (count != nullptr) {
    *count = std::min<unsigned>(*count, axes.size());
    for (unsigned index = 0; index < *count; ++index) infos[index] = axes[index];
  }
  return axes.size();
}

HbSubsetInput *inputCreate() {
  return fake.inputFails ? nullptr : std::bit_cast<HbSubsetInput *>(&fake.objects[2]);
}

void inputDestroy(HbSubsetInput *) {}

void inputKeepEverything(HbSubsetInput *) {}

int pinAxisLocation(HbSubsetInput *, HbFace *, std::uint32_t, float) {
  ++fake.pinned;
  return 1;
}

int pinAxisToDefault(HbSubsetInput *, HbFace *, std::uint32_t) {
  ++fake.defaulted;
  return 1;
}

HbFace *subset(HbFace *, const HbSubsetInput *) {
  ++fake.subsets;
  return fake.subsets == fake.failingSubset ? nullptr : std::bit_cast<HbFace *>(&fake.objects[1]);
}

constexpr HarfBuzzSubset kFakeHarfBuzz{
    .blobCreateFromFile = blobCreateFromFile,
    .blobDestroy = blobDestroy,
    .blobData = blobData,
    .faceCreate = faceCreate,
    .faceDestroy = faceDestroy,
    .faceBlob = faceBlob,
    .axisInfos = axisInfos,
    .inputCreate = inputCreate,
    .inputDestroy = inputDestroy,
    .inputKeepEverything = inputKeepEverything,
    .pinAxisLocation = pinAxisLocation,
    .pinAxisToDefault = pinAxisToDefault,
    .subset = subset,
};

struct Workspace {
  Workspace() {
    fake = {};
    fs::remove_all(root);
    fs::create_directories(root / "instances");
    std::ofstream(font) << "variable";
  }
  ~Workspace() {
    fs::remove_all(root);
  }
  Workspace(const Workspace &) = delete;
  Workspace &operator=(const Workspace &) = delete;

  const fs::path root{fs::temp_directory_path() / "rn-tizen-font-instances-test"};
  const fs::path font{root / "Brand.ttf"};
  const fs::path directory{root / "instances"};
};

} // namespace

TEST_CASE_FIXTURE(Workspace, "writeWeightInstances writes one font per hundred the weight axis covers, then reuses them") {
  const auto files = writeWeightInstances(kFakeHarfBuzz, font, directory);
  const auto again = writeWeightInstances(kFakeHarfBuzz, font, directory);

  REQUIRE(files.size() == 5);
  CHECK(files.front() == directory / "Brand-300.ttf");
  CHECK(files.back() == directory / "Brand-700.ttf");
  CHECK(fs::file_size(files.front()) == 4);
  CHECK(again == files);
  CHECK(fake.subsets == 5);
  CHECK(fake.pinned == 5);
  CHECK(fake.defaulted == 5);
}

TEST_CASE_FIXTURE(Workspace, "writeWeightInstances removes what it wrote when HarfBuzz fails part way") {
  fake.failingSubset = 3;

  const auto files = writeWeightInstances(kFakeHarfBuzz, font, directory);

  CHECK(files.empty());
  CHECK(fs::is_empty(directory));
}

TEST_CASE_FIXTURE(Workspace, "writeWeightInstances writes nothing without a weight axis, a font or a subset input") {
  CHECK(writeWeightInstances(kFakeHarfBuzz, root / "Missing.ttf", directory).empty());
  fake.inputFails = true;
  CHECK(writeWeightInstances(kFakeHarfBuzz, font, directory).empty());
  fake.hasWeightAxis = false;
  CHECK(writeWeightInstances(kFakeHarfBuzz, font, directory).empty());
}

TEST_CASE("writeWeightInstances with the system's HarfBuzz writes nothing for a font it cannot instance") {
  const auto directory = fs::temp_directory_path() / "rn-tizen-font-instances-system";
  const fs::path staticFont = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";

  CHECK(writeWeightInstances("/nonexistent/Font.ttf", directory).empty());
  if (fs::exists(staticFont)) CHECK(writeWeightInstances(staticFont, directory).empty());
}

} // namespace rntizen
