#include "AlphaBlur.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace rntizen {

namespace {

constexpr int kSide = 96;

std::vector<float> square(int side, int from, int to) {
  std::vector<float> alpha(static_cast<std::size_t>(side * side), 0.0f);
  for (int y = from; y < to; ++y) {
    for (int x = from; x < to; ++x) alpha[static_cast<std::size_t>(y * side + x)] = 1;
  }
  return alpha;
}

std::vector<float> referenceGaussian(const std::vector<float> &alpha, int side, float sigma, int radius) {
  std::vector<float> kernel;
  for (int i = -radius; i <= radius; ++i) kernel.push_back(std::exp(-static_cast<float>(i * i) / (2 * sigma * sigma)));
  float sum = 0;
  for (const auto weight : kernel) sum += weight;
  for (auto &weight : kernel) weight /= sum;
  auto pass = [&kernel, radius, side](const std::vector<float> &source, int stepX, int stepY) {
    std::vector<float> target(source.size(), 0.0f);
    for (int y = 0; y < side; ++y) {
      for (int x = 0; x < side; ++x) {
        float total = 0;
        for (int i = -radius; i <= radius; ++i) {
          const auto sx = x + i * stepX;
          const auto sy = y + i * stepY;
          if (sx >= 0 && sy >= 0 && sx < side && sy < side) {
            total += source[static_cast<std::size_t>(sy * side + sx)] * kernel[static_cast<std::size_t>(i + radius)];
          }
        }
        target[static_cast<std::size_t>(y * side + x)] = total;
      }
    }
    return target;
  };
  return pass(pass(alpha, 1, 0), 0, 1);
}

float largestDifference(const std::vector<float> &a, const std::vector<float> &b) {
  float largest = 0;
  for (std::size_t i = 0; i < a.size(); ++i) largest = std::max(largest, std::abs(a[i] - b[i]));
  return largest;
}

float total(const std::vector<float> &alpha) {
  float sum = 0;
  for (const auto value : alpha) sum += value;
  return sum;
}

} // namespace

TEST_CASE("The three box blurs stay within 4% of a true Gaussian") {
  for (const float sigma : {3.0f, 5.0f, 12.0f, 20.0f}) {
    const auto alpha = square(kSide, 30, 66);

    const auto blurred = blurredAlpha(alpha, kSide, kSide, static_cast<int>(std::ceil(sigma * 3)), sigma);

    CAPTURE(sigma);
    CHECK(largestDifference(blurred, referenceGaussian(alpha, kSide, sigma, static_cast<int>(std::ceil(sigma * 4)))) < 0.04f);
  }
}

TEST_CASE("A small blur is the Gaussian kernel itself") {
  const auto alpha = square(kSide, 40, 56);

  const auto blurred = blurredAlpha(alpha, kSide, kSide, 6, 2);

  CHECK(largestDifference(blurred, referenceGaussian(alpha, kSide, 2, 6)) < 1e-6f);
}

TEST_CASE("Blurring keeps the total alpha when nothing reaches the edge") {
  const auto alpha = square(kSide, 30, 66);

  const auto blurred = blurredAlpha(alpha, kSide, kSide, 15, 5);

  CHECK(total(blurred) == doctest::Approx(total(alpha)).epsilon(1e-3));
}

TEST_CASE("Blurring is symmetric about the shape's center") {
  const auto alpha = square(kSide, 30, 66);

  const auto blurred = blurredAlpha(alpha, kSide, kSide, 24, 8);

  for (int x = 0; x < kSide; ++x) {
    const auto mirrored = kSide - 1 - x;
    CHECK(blurred[static_cast<std::size_t>(48 * kSide + x)] ==
          doctest::Approx(blurred[static_cast<std::size_t>(48 * kSide + mirrored)]).epsilon(1e-4));
  }
}

} // namespace rntizen
