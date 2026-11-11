#pragma once

#include "../../views/ExpoView.h"

#include "platform/MainLoop.h"
#include "views/ImageContent.h"
#include "views/ImageDecoder.h"

#include <memory>
#include <string>

namespace rnwebos::expo {

/// expo-image's `Image` view: the first source, downloaded through the disk
/// cache and decoded off the main thread (at about the drawn size when the
/// image is twice it or more), then fitted by `contentFit` and faded in over
/// `transition`.
class ExpoImageView final : public ExpoView {
 public:
  ExpoImageView(Scene &scene, facebook::react::Tag tag);
  ~ExpoImageView() override;

  void setSource(const folly::dynamic &value);
  void setContentFit(const folly::dynamic &value);
  void setTransition(const folly::dynamic &value);

 protected:
  void didUpdateProps() override;
  void layoutContent() override;
  void drawContent(SkCanvas &canvas, float alpha) override;
  int contentLayers() const override;

 private:
  void loadIfNeeded();
  void load();
  void sharpenIfNeeded();
  void decode(PixelSize box, bool quiet);
  void display(const DecodedImage &decoded, bool quiet);
  void fail(const std::string &message, unsigned int request);
  const std::string &memoryKey() const;
  PixelRect box() const;
  void fadeIn();
  void stopFade();

  ImageContent content_;
  std::string uri_;
  std::string cacheKey_;
  std::string path_;
  std::string cacheType_;
  std::shared_ptr<ImageDecode> decode_;
  double transitionSeconds_{0};
  bool sourceChanged_{false};
  unsigned int request_{0};
  MainLoop::Id fade_{0};
  MainLoop::Clock::time_point fadeStarted_;
  float fadeAlpha_{1};
  std::shared_ptr<ExpoImageView *> alive_;
};

} // namespace rnwebos::expo
