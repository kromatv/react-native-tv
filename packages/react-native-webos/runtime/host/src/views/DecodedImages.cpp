#include "DecodedImages.h"

#include "include/gpu/ganesh/SkImageGanesh.h"

#include <cstddef>
#include <list>

namespace rnwebos {

namespace {

constexpr std::size_t kBudgetBytes = 64u << 20;

struct Kept {
  std::string key;
  DecodedImage image;
  std::size_t bytes;
};

struct Cache {
  std::list<Kept> recent;
  std::size_t bytes{0};
};

Cache &cache() {
  static Cache shared;
  return shared;
}

bool sharpEnough(const DecodedImage &kept, ImageFit fit, PixelSize box, float naturalScale) {
  const auto wanted = decodedImageSize(fit, box, kept.size, naturalScale);
  return kept.image->width() >= wanted.width && kept.image->height() >= wanted.height;
}

sk_sp<SkImage> uploaded(Scene &scene, const sk_sp<SkImage> &image) {
  auto *window = scene.window();
  if (window == nullptr) return image;
  auto texture = SkImages::TextureFromImage(&window->gpu(), image.get());
  return texture ? texture : image;
}

} // namespace

std::optional<DecodedImage> findDecodedImage(const std::string &key, ImageFit fit, PixelSize box, float naturalScale) {
  auto &kept = cache().recent;
  for (auto entry = kept.begin(); entry != kept.end(); ++entry) {
    if (entry->key != key || !sharpEnough(entry->image, fit, box, naturalScale)) continue;
    kept.splice(kept.begin(), kept, entry);
    return entry->image;
  }
  return std::nullopt;
}

DecodedImage keepDecodedImage(Scene &scene, const std::string &key, DecodedImage image) {
  if (!image.image) return image;
  image.image = uploaded(scene, image.image);
  auto &shared = cache();
  const auto bytes = static_cast<std::size_t>(image.image->width()) * static_cast<std::size_t>(image.image->height()) * 4;
  if (bytes > kBudgetBytes / 4) return image;
  shared.recent.push_front({.key = key, .image = image, .bytes = bytes});
  shared.bytes += bytes;
  while (shared.bytes > kBudgetBytes && shared.recent.size() > 1) {
    shared.bytes -= shared.recent.back().bytes;
    shared.recent.pop_back();
  }
  return image;
}

void clearDecodedImages() {
  cache() = {};
}

} // namespace rnwebos
