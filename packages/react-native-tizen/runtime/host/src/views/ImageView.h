#pragma once

#include "ComponentView.h"
#include "ImageFetcher.h"

#include <react/renderer/components/image/ImageEventEmitter.h>
#include <react/renderer/imagemanager/primitives.h>

#include <memory>
#include <string>

namespace rntizen {

/// An `<Image>`: a remote (`http`, `https`), `data:` or file source, decoded
/// by Evas in the background while the image object stays hidden, then shown
/// fitted by `resizeMode` inside the padding and clipped to rounded corners.
/// Emits `onLoadStart`, then `onLoad` or `onError`, then `onLoadEnd`, when
/// `shouldNotifyLoadEvents` asks for them, as on Android.
class ImageView final : public ComponentView {
 public:
  ImageView(Evas *evas, facebook::react::Tag tag);
  ~ImageView() override;

  void updateProps(const facebook::react::Props::Shared &props) override;
  void updateState(const facebook::react::State::Shared &state) override;
  void updateLayout(const facebook::react::LayoutMetrics &layoutMetrics) override;

 protected:
  void layoutMembers() override;

 private:
  static void onPreloaded(void *data, Evas *evas, Evas_Object *object, void *info);

  void loadIfNeeded();
  void startLoad();
  void decode(const FetchedImage &fetched);
  void setSource(const FetchedImage &fetched);
  void finishLoad();
  void fail(const std::string &message, int status);
  void updateCorners();
  const facebook::react::ImageEventEmitter *loadEvents() const;

  Evas_Object *image_;
  facebook::react::ImageSource source_;
  std::string requestedUri_;
  std::shared_ptr<ImageFetch> fetch_;
  facebook::react::ImageResizeMode resizeMode_{facebook::react::ImageResizeMode::Cover};
  int imageWidth_{0};
  int imageHeight_{0};
  bool notifiesLoadEvents_{false};
  bool hasStateSource_{false};
  bool decoding_{false};
  bool loaded_{false};
};

} // namespace rntizen
