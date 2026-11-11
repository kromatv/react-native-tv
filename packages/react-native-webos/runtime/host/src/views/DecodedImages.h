#pragma once

#include "../render/Scene.h"
#include "ImageDecoder.h"

#include <optional>
#include <string>

namespace rnwebos {

/// The kept image of `key`, a source's URI or cache key, when it is sharp
/// enough to draw in `box` with `fit`. Main thread only.
std::optional<DecodedImage> findDecodedImage(const std::string &key, ImageFit fit, PixelSize box, float naturalScale);

/// Keeps `image` under `key` and returns it as kept: as a texture of the GPU
/// `scene` draws with, uploaded now so no frame pays for it and the decoded
/// pixels are freed (docs/reference/skia-rendering.md, section 7). The images
/// kept stay under a budget, the least recently shown going first, so a view
/// mounted again shows its picture at once. Main thread only.
DecodedImage keepDecodedImage(Scene &scene, const std::string &key, DecodedImage image);

/// Forgets every kept image; views keep the ones they show.
void clearDecodedImages();

} // namespace rnwebos
