#include "PatchCache.h"

#include <doctest/doctest.h>

namespace rntizen {

namespace {

constexpr std::size_t kPixelBytes = 4;

NinePatch patchOf(int pixels) {
  NinePatch patch{.width = pixels, .height = 1};
  patch.pixels.resize(static_cast<std::size_t>(pixels));
  return patch;
}

PatchKey keyOf(float id) {
  return {id};
}

} // namespace

TEST_CASE("PatchCache finds what it stored and nothing else") {
  PatchCache cache(1024);

  cache.insert(keyOf(1), patchOf(4));

  REQUIRE(cache.find(keyOf(1)) != nullptr);
  CHECK(cache.find(keyOf(1))->width == 4);
  CHECK(cache.find(keyOf(2)) == nullptr);
  CHECK(cache.bytes() == 4 * kPixelBytes);
}

TEST_CASE("PatchCache drops the least recently used patch past its budget") {
  PatchCache cache(10 * kPixelBytes);
  cache.insert(keyOf(1), patchOf(4));
  cache.insert(keyOf(2), patchOf(4));

  cache.find(keyOf(1));
  cache.insert(keyOf(3), patchOf(4));

  CHECK(cache.find(keyOf(1)) != nullptr);
  CHECK(cache.find(keyOf(2)) == nullptr);
  CHECK(cache.find(keyOf(3)) != nullptr);
  CHECK(cache.bytes() == 8 * kPixelBytes);
}

TEST_CASE("PatchCache keeps the newest patch even when it alone passes the budget") {
  PatchCache cache(2 * kPixelBytes);
  cache.insert(keyOf(1), patchOf(1));

  const auto &stored = cache.insert(keyOf(2), patchOf(8));

  CHECK(stored.width == 8);
  CHECK(cache.find(keyOf(1)) == nullptr);
  CHECK(cache.find(keyOf(2)) != nullptr);
  CHECK(cache.bytes() == 8 * kPixelBytes);
}

TEST_CASE("PatchCache replaces a patch stored again under the same key") {
  PatchCache cache(1024);
  cache.insert(keyOf(1), patchOf(4));

  cache.insert(keyOf(1), patchOf(2));

  CHECK(cache.find(keyOf(1))->width == 2);
  CHECK(cache.bytes() == 2 * kPixelBytes);
}

TEST_CASE("patchKey starts with the kind and the eight radii, and appendEdges adds four widths") {
  const Corners corners{.topLeft = {1, 2}, .topRight = {3, 4}, .bottomRight = {5, 6}, .bottomLeft = {7, 8}};

  auto key = patchKey(PatchKind::Ring, corners);
  appendEdges(key, {.left = 9, .top = 10, .right = 11, .bottom = 12});

  CHECK(key == PatchKey{static_cast<float>(PatchKind::Ring), 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  CHECK(patchKey(PatchKind::Filled, corners) != patchKey(PatchKind::Shadow, corners));
}

TEST_CASE("cachedPatch paints a key once") {
  int painted = 0;
  const PatchKey key{-42};

  cachedPatch(key, [&painted] {
    ++painted;
    return NinePatch{.width = 1, .height = 1, .border = {}, .pixels = {0}};
  });
  const auto &again = cachedPatch(key, [&painted] {
    ++painted;
    return NinePatch{};
  });

  CHECK(painted == 1);
  CHECK(again.width == 1);
}

} // namespace rntizen
