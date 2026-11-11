#include "ImageDecoder.h"

#include "../threading/MainThread.h"

#include <folly/Indestructible.h>

#include "include/codec/SkCodec.h"
#include "include/codec/SkEncodedOrigin.h"
#include "include/codec/SkGifDecoder.h"
#include "include/codec/SkJpegDecoder.h"
#include "include/codec/SkPixmapUtils.h"
#include "include/codec/SkPngDecoder.h"
#include "include/codec/SkWebpDecoder.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkData.h"
#include "include/core/SkSamplingOptions.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <format>
#include <mutex>
#include <thread>
#include <vector>

namespace rnwebos {

class ImageDecode {
 public:
  explicit ImageDecode(std::function<void(DecodedImage)> done) : done_(std::move(done)) {}

  void deliver(DecodedImage image) const {
    done_(std::move(image));
  }

 private:
  std::function<void(DecodedImage)> done_;
};

namespace {

constexpr int kWorkerCount = 2;
constexpr std::int64_t kMostDecodedPixels = 40'000'000;

const std::array<SkCodecs::Decoder, 4> kDecoders{
    SkPngDecoder::Decoder(), SkJpegDecoder::Decoder(), SkWebpDecoder::Decoder(), SkGifDecoder::Decoder()};

class DecodePool final {
 public:
  DecodePool() {
    workers_.reserve(kWorkerCount);
    for (int index = 0; index < kWorkerCount; ++index) workers_.emplace_back([this] { run(); });
  }

  void push(std::function<void()> job) {
    {
      std::lock_guard lock(mutex_);
      jobs_.push_back(std::move(job));
    }
    wake_.notify_one();
  }

 private:
  std::function<void()> takeJob() {
    std::unique_lock lock(mutex_);
    wake_.wait(lock, [this] { return !jobs_.empty(); });
    auto job = std::move(jobs_.front());
    jobs_.pop_front();
    return job;
  }

  [[noreturn]] void run() {
    while (true) {
      const auto job = takeJob();
      job();
    }
  }

  std::mutex mutex_;
  std::condition_variable wake_;
  std::deque<std::function<void()>> jobs_;
  std::vector<std::jthread> workers_;
};

DecodePool &pool() {
  static folly::Indestructible<DecodePool> shared;
  return *shared;
}

DecodedImage failed(std::string error, PixelSize size = {}) {
  return {.image = nullptr, .size = size, .error = std::move(error)};
}

std::unique_ptr<SkCodec> codecFor(const sk_sp<SkData> &data) {
  return SkCodec::MakeFromData(data, kDecoders);
}

PixelSize uprightSize(const SkCodec &codec) {
  const auto encoded = codec.dimensions();
  if (SkEncodedOriginSwapsWidthHeight(codec.getOrigin())) return {encoded.height(), encoded.width()};
  return {encoded.width(), encoded.height()};
}

// The size to ask the codec for: `target` or the nearest size above it the
// codec scales to while it decodes. Skia's JPEG codec snaps to the nearest
// eighth, so the request is rounded up to one.
SkISize codecSize(const SkCodec &codec, SkISize target) {
  const auto encoded = codec.dimensions();
  const auto scale = std::max(
      static_cast<float>(target.width()) / static_cast<float>(encoded.width()),
      static_cast<float>(target.height()) / static_cast<float>(encoded.height()));
  if (scale >= 1) return encoded;
  const auto scaled = codec.getScaledDimensions(std::ceil(scale * 8) / 8);
  return scaled.width() >= target.width() && scaled.height() >= target.height() ? scaled : encoded;
}

bool shrink(SkBitmap &pixels, SkISize target) {
  if (pixels.width() <= target.width() && pixels.height() <= target.height()) return true;
  SkBitmap smaller;
  if (!smaller.tryAllocPixels(pixels.info().makeDimensions(target))) return false;
  if (!pixels.pixmap().scalePixels(smaller.pixmap(), SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kLinear))) {
    return false;
  }
  pixels = smaller;
  return true;
}

bool turnUpright(SkBitmap &pixels, SkEncodedOrigin origin) {
  if (origin == kTopLeft_SkEncodedOrigin) return true;
  const auto info = SkEncodedOriginSwapsWidthHeight(origin) ? SkPixmapUtils::SwapWidthHeight(pixels.info()) : pixels.info();
  SkBitmap upright;
  if (!upright.tryAllocPixels(info) || !SkPixmapUtils::Orient(upright.pixmap(), pixels.pixmap(), origin)) return false;
  pixels = upright;
  return true;
}

} // namespace

DecodedImage decodeImageNow(const ImageDecodeRequest &request) {
  const auto data = request.bytes ? SkData::MakeWithoutCopy(request.bytes->data(), request.bytes->size())
                                  : SkData::MakeFromFileName(request.path.c_str());
  if (!data) return failed("Cannot read " + request.path);
  const auto codec = codecFor(data);
  if (!codec) return failed("Not a PNG, JPEG, WebP or GIF image");
  const auto size = uprightSize(*codec);
  const auto origin = codec->getOrigin();
  const auto wanted = decodedImageSize(request.fit, request.box, size, request.naturalScale);
  const auto target = SkEncodedOriginSwapsWidthHeight(origin) ? SkISize::Make(wanted.height, wanted.width)
                                                              : SkISize::Make(wanted.width, wanted.height);
  const auto decodeSize = codecSize(*codec, target);
  if (static_cast<std::int64_t>(decodeSize.width()) * decodeSize.height() > kMostDecodedPixels) {
    return failed(std::format("The image is too large to decode: {}x{}", size.width, size.height), size);
  }
  // RGBA, because an OpenGL ES 2 TV takes BGRA textures only with an
  // extension (docs/reference/skia-rendering.md, section 1).
  const auto alphaType = codec->getInfo().isOpaque() ? kOpaque_SkAlphaType : kPremul_SkAlphaType;
  SkBitmap pixels;
  if (!pixels.tryAllocPixels(SkImageInfo::Make(decodeSize, kRGBA_8888_SkColorType, alphaType, SkColorSpace::MakeSRGB()))) {
    return failed("Out of memory for the image's pixels", size);
  }
  const auto result = codec->getPixels(pixels.pixmap());
  if (result != SkCodec::kSuccess && result != SkCodec::kIncompleteInput && result != SkCodec::kErrorInInput) {
    return failed(std::format("Cannot decode the image: {}", SkCodec::ResultToString(result)), size);
  }
  if (!shrink(pixels, target) || !turnUpright(pixels, origin)) return failed("Out of memory for the image's pixels", size);
  pixels.setImmutable();
  return {.image = pixels.asImage(), .size = size, .error = {}};
}

PixelSize encodedImageSize(const std::string &bytes) {
  const auto codec = codecFor(SkData::MakeWithoutCopy(bytes.data(), bytes.size()));
  return codec ? uprightSize(*codec) : PixelSize{};
}

std::shared_ptr<ImageDecode> decodeImage(ImageDecodeRequest request, std::function<void(DecodedImage)> done) {
  auto decode = std::make_shared<ImageDecode>(std::move(done));
  pool().push([request = std::move(request), waiting = std::weak_ptr<ImageDecode>(decode)] {
    if (waiting.expired()) return;
    runOnMainThread([waiting, image = decodeImageNow(request)]() mutable {
      if (const auto alive = waiting.lock()) alive->deliver(std::move(image));
    });
  });
  return decode;
}

} // namespace rnwebos
