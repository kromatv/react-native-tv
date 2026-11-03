#include "ScreenCapture.h"

#include "../HostWindow.h"
#include "../threading/MainThread.h"

#include <Evas.h>

#include <algorithm>
#include <bit>
#include <cstring>
#include <memory>
#include <utility>

namespace rntizen {

namespace {

static_assert(std::endian::native == std::endian::little, "ARGB8888 words must read as BGRA bytes");

constexpr int kFramesToWait = 10;

struct CaptureJob {
  Evas_Object *snapshot;
  int width;
  int height;
  CaptureDone done;
  int framesLeft{kFramesToWait};
};

Evas_Object *addSnapshot(Evas *evas, int width, int height) {
  auto *snapshot = evas_object_image_filled_add(evas);
  evas_object_image_snapshot_set(snapshot, EINA_TRUE);
  evas_object_pass_events_set(snapshot, EINA_TRUE);
  evas_object_layer_set(snapshot, EVAS_LAYER_MAX);
  evas_object_geometry_set(snapshot, 0, 0, width, height);
  evas_object_show(snapshot);
  evas_object_raise(snapshot);
  return snapshot;
}

std::optional<ScreenCapture> readSnapshot(Evas_Object *snapshot) {
  auto *data = static_cast<std::uint8_t *>(evas_object_image_data_get(snapshot, EINA_FALSE));
  if (data == nullptr) return std::nullopt;
  int width = 0;
  int height = 0;
  evas_object_image_size_get(snapshot, &width, &height);
  const auto rowBytes = static_cast<std::size_t>(width) * 4;
  const auto stride = std::max(static_cast<std::size_t>(evas_object_image_stride_get(snapshot)), rowBytes);
  ScreenCapture capture{
      .width = width,
      .height = height,
      .stride = static_cast<int>(rowBytes),
      .pixels = std::vector<std::uint8_t>(rowBytes * static_cast<std::size_t>(height)),
  };
  for (std::size_t row = 0; row < static_cast<std::size_t>(height); ++row) {
    std::memcpy(capture.pixels.data() + row * rowBytes, data + row * stride, rowBytes);
  }
  evas_object_image_data_set(snapshot, data);
  return capture;
}

void onRenderPost(void *data, Evas *evas, void *) {
  auto *job = static_cast<CaptureJob *>(data);
  auto capture = readSnapshot(job->snapshot);
  if (!capture.has_value()) --job->framesLeft;
  if (!capture.has_value() && job->framesLeft > 0) {
    evas_object_del(job->snapshot);
    job->snapshot = addSnapshot(evas, job->width, job->height);
    return;
  }
  evas_event_callback_del_full(evas, EVAS_CALLBACK_RENDER_POST, onRenderPost, job);
  runOnMainThread([job, capture = std::move(capture)]() mutable {
    const std::unique_ptr<CaptureJob> finished(job);
    evas_object_del(finished->snapshot);
    finished->done(std::move(capture));
  });
}

} // namespace

void captureHostWindow(CaptureDone done) {
  auto *window = hostWindow();
  if (window == nullptr) {
    done(std::nullopt);
    return;
  }
  auto *evas = evas_object_evas_get(window);
  int width = 0;
  int height = 0;
  evas_object_geometry_get(window, nullptr, nullptr, &width, &height);
  auto job = std::make_unique<CaptureJob>(CaptureJob{
      .snapshot = addSnapshot(evas, width, height), .width = width, .height = height, .done = std::move(done)});
  evas_event_callback_add(evas, EVAS_CALLBACK_RENDER_POST, onRenderPost, job.release());
}

} // namespace rntizen
