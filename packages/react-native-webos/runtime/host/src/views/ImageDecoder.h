#pragma once

#include "ImageFit.h"

#include "include/core/SkImage.h"

#include <functional>
#include <memory>
#include <string>

namespace rnwebos {

/// An encoded image and the box it is decoded for.
struct ImageDecodeRequest {
  /// The encoded bytes, or null to read them from `path`.
  std::shared_ptr<const std::string> bytes;
  std::string path;
  ImageFit fit{ImageFit::Stretch};
  PixelSize box;
  float naturalScale{1};
};

/// A decoded image, or why there is none.
struct DecodedImage {
  /// RGBA pixels in sRGB, at `decodedImageSize` of the request.
  sk_sp<SkImage> image;
  /// The size the image has in its file, upright.
  PixelSize size;
  std::string error;
};

/// A decode in flight. Dropping it drops the result, and skips the decode
/// when it has not started.
class ImageDecode;

/// Decodes a PNG, JPEG, WebP or GIF (its first frame) on one of a few
/// background threads, scaled down while decoding when it is much larger than
/// its box. `done` runs on the main loop while the returned decode is alive.
std::shared_ptr<ImageDecode> decodeImage(ImageDecodeRequest request, std::function<void(DecodedImage)> done);

/// What `decodeImage` delivers, decoded on the calling thread.
DecodedImage decodeImageNow(const ImageDecodeRequest &request);

/// The size of the image in `bytes`, upright, without decoding its pixels;
/// empty when it is not an image the host decodes.
PixelSize encodedImageSize(const std::string &bytes);

} // namespace rnwebos
