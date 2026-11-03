#include "AlphaBlur.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <utility>

namespace rntizen {

namespace {

std::vector<float> gaussianKernel(int radius, float sigma) {
  std::vector<float> kernel(static_cast<std::size_t>(2 * radius + 1));
  float sum = 0;
  for (int i = -radius; i <= radius; ++i) {
    const auto weight = std::exp(-static_cast<float>(i * i) / (2 * sigma * sigma));
    kernel[static_cast<std::size_t>(i + radius)] = weight;
    sum += weight;
  }
  for (auto &weight : kernel) weight /= sum;
  return kernel;
}

std::vector<float> gaussianBlurred(const std::vector<float> &alpha, int width, int height, int radius, float sigma) {
  const auto kernel = gaussianKernel(radius, sigma);
  auto pass = [&](const std::vector<float> &source, int stepX, int stepY) {
    std::vector<float> target(source.size());
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        float sum = 0;
        for (int i = -radius; i <= radius; ++i) {
          const int sx = x + i * stepX;
          const int sy = y + i * stepY;
          if (sx < 0 || sy < 0 || sx >= width || sy >= height) continue;
          sum += source[static_cast<std::size_t>(sy * width + sx)] * kernel[static_cast<std::size_t>(i + radius)];
        }
        target[static_cast<std::size_t>(y * width + x)] = sum;
      }
    }
    return target;
  };
  return pass(pass(alpha, 1, 0), 0, 1);
}

constexpr int kLanes = 4;

void horizontalPass(
    const std::vector<float> &source, std::vector<float> &target, int width, int height, int before, int after) {
  const auto scale = 1 / static_cast<float>(before + after + 1);
  const auto offset = before + 1;
  const auto stride = static_cast<std::size_t>(width + before + after + 2);
  std::vector<float> padded(stride * kLanes, 0.0f);
  for (int y = 0; y < height; y += kLanes) {
    const int lanes = std::min(kLanes, height - y);
    std::array<const float *, kLanes> at{};
    std::array<float *, kLanes> out{};
    std::array<float, kLanes> sum{};
    for (int lane = 0; lane < lanes; ++lane) {
      const auto *line = source.data() + static_cast<std::size_t>(y + lane) * static_cast<std::size_t>(width);
      std::copy(line, line + width, padded.begin() + static_cast<std::ptrdiff_t>(stride * lane) + offset);
      at[lane] = padded.data() + stride * lane + offset;
      out[lane] = target.data() + static_cast<std::size_t>(y + lane) * static_cast<std::size_t>(width);
      for (int i = -before; i <= after; ++i) sum[lane] += at[lane][i];
    }
    for (int i = 0; i < width; ++i) {
      for (int lane = 0; lane < lanes; ++lane) {
        out[lane][i] = sum[lane] * scale;
        sum[lane] += at[lane][i + after + 1] - at[lane][i - before];
      }
    }
  }
}

void verticalPass(
    const std::vector<float> &source, std::vector<float> &target, int width, int height, int before, int after) {
  const auto scale = 1 / static_cast<float>(before + after + 1);
  const auto row = [&](int y) { return source.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(width); };
  std::vector<float> sums(static_cast<std::size_t>(width), 0.0f);
  for (int i = -before; i <= after; ++i) {
    if (i < 0 || i >= height) continue;
    const auto *line = row(i);
    for (int x = 0; x < width; ++x) sums[static_cast<std::size_t>(x)] += line[x];
  }
  for (int i = 0; i < height; ++i) {
    auto *out = target.data() + static_cast<std::size_t>(i) * static_cast<std::size_t>(width);
    for (int x = 0; x < width; ++x) out[x] = sums[static_cast<std::size_t>(x)] * scale;
    const auto *entering = i + after + 1 < height ? row(i + after + 1) : nullptr;
    const auto *leaving = i - before >= 0 ? row(i - before) : nullptr;
    auto *sum = sums.data();
    if (entering != nullptr && leaving != nullptr) {
      for (int x = 0; x < width; ++x) sum[x] += entering[x] - leaving[x];
    } else if (entering != nullptr) {
      for (int x = 0; x < width; ++x) sum[x] += entering[x] - 0.0f;
    } else if (leaving != nullptr) {
      for (int x = 0; x < width; ++x) sum[x] += 0.0f - leaving[x];
    }
  }
}

std::vector<float> boxBlurred(std::vector<float> alpha, int width, int height, float sigma) {
  const auto size = static_cast<int>(std::floor(sigma * 3 * std::sqrt(2 * std::numbers::pi_v<float>) / 4 + 0.5f));
  const auto half = size / 2;
  const std::array<std::array<int, 2>, 3> boxes = size % 2 == 1
      ? std::array<std::array<int, 2>, 3>{{{half, half}, {half, half}, {half, half}}}
      : std::array<std::array<int, 2>, 3>{{{half, half - 1}, {half - 1, half}, {half, half}}};
  std::vector<float> scratch(alpha.size());
  for (const auto &[before, after] : boxes) {
    horizontalPass(alpha, scratch, width, height, before, after);
    alpha.swap(scratch);
  }
  for (const auto &[before, after] : boxes) {
    verticalPass(alpha, scratch, width, height, before, after);
    alpha.swap(scratch);
  }
  return alpha;
}

} // namespace

std::vector<float> blurredAlpha(std::vector<float> alpha, int width, int height, int radius, float sigma) {
  if (sigma >= 3) return boxBlurred(std::move(alpha), width, height, sigma);
  return gaussianBlurred(alpha, width, height, radius, sigma);
}

} // namespace rntizen
