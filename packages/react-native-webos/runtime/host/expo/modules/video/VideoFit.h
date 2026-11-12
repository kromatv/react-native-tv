#pragma once

#include "PlayerModel.h"

#include "views/PixelGeometry.h"

#include <optional>

namespace rnwebos::expo {

/// The part of the decoded frame the plane shows, in 0..1 ratios of its size.
struct SourceCrop {
  double x{0};
  double y{0};
  double width{1};
  double height{1};

  bool operator==(const SourceCrop &) const = default;
};

/// Where the video plane goes: its destination in window pixels and the source
/// crop that fills it.
struct PlaneGeometry {
  PixelRect roi;
  SourceCrop crop;

  bool operator==(const PlaneGeometry &) const = default;
};

/// Fits a `videoWidth` by `videoHeight` frame into `view` by `fit` and keeps
/// what lands inside `window`, since the plane stretches the crop over its
/// destination. Nullopt when nothing of it is on screen. An unknown video size
/// fills the view.
std::optional<PlaneGeometry> fitVideo(
    const PixelRect &view, const PixelRect &window, int videoWidth, int videoHeight, ContentFit fit);

/// `crop` of a `videoWidth` by `videoHeight` frame in frame pixels, which is
/// what the plane's source rectangle is in; empty while the size is unknown.
PixelRect sourcePixels(const SourceCrop &crop, int videoWidth, int videoHeight);

} // namespace rnwebos::expo
