#pragma once

#include "ComponentView.h"
#include "ImageContent.h"
#include "ImageDecoder.h"
#include "ImageFetcher.h"

#include <react/renderer/components/image/ImageEventEmitter.h>
#include <react/renderer/imagemanager/primitives.h>

#include <memory>
#include <string>

namespace rnwebos {

/// An `<Image>`: a remote (`http`, `https`), `data:` or file source, fetched
/// and decoded off the main thread, then painted fitted by `resizeMode`
/// inside the padding, tinted by `tintColor` and clipped to rounded corners.
/// Emits `onLoadStart`, then `onLoad` or `onError`, then `onLoadEnd`, when
/// `shouldNotifyLoadEvents` asks for them, as on Android.
class ImageView final : public ComponentView {
 public:
  using ComponentView::ComponentView;

  void updateProps(const facebook::react::Props::Shared &props) override;
  void updateState(const facebook::react::State::Shared &state) override;
  void updateLayout(const facebook::react::LayoutMetrics &layoutMetrics) override;

 private:
  void drawContent(SkCanvas &canvas, float alpha) override;
  int contentLayers() const override;
  void layoutContent() override;

  void loadIfNeeded();
  void startLoad();
  void sharpenIfNeeded();
  void request(PixelSize box);
  void decode(ImageDecodeRequest request);
  void show(const DecodedImage &decoded);
  void fail(const std::string &message, int status);
  void updateCorners();
  PixelRect contentBox() const;
  const facebook::react::ImageEventEmitter *loadEvents() const;

  ImageContent content_;
  facebook::react::ImageSource source_;
  std::string requestedUri_;
  std::shared_ptr<ImageFetch> fetch_;
  std::shared_ptr<ImageDecode> decode_;
  bool notifiesLoadEvents_{false};
  bool hasStateSource_{false};
  bool sharpening_{false};
};

} // namespace rnwebos
