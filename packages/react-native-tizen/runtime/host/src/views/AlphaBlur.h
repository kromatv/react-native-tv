#pragma once

#include <vector>

namespace rntizen {

/// `alpha`, `width` x `height` values, blurred by a Gaussian of deviation
/// `sigma`, zero outside. From a deviation of 3, three box blurs per axis
/// stand in for it as W3C Filter Effects' feGaussianBlur describes, at a cost
/// that does not grow with the radius; below, a kernel of `radius` pixels each
/// side.
std::vector<float> blurredAlpha(std::vector<float> alpha, int width, int height, int radius, float sigma);

} // namespace rntizen
