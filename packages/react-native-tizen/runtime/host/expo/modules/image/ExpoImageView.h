#pragma once

#include "../../views/ExpoView.h"

#include <Ecore.h>

#include <memory>
#include <string>

namespace rntizen::expo {

/// expo-image's `Image` view: the first source, downloaded through the disk
/// cache off the main thread, decoded by Evas's preload thread while hidden
/// (at about the drawn size when the image is twice it or more), then fitted
/// by `contentFit` and shown.
class ExpoImageView final : public ExpoView {
 public:
  ExpoImageView(Evas *evas, facebook::react::Tag tag);
  ~ExpoImageView() override;

  void setSource(const folly::dynamic &value);
  void setContentFit(const folly::dynamic &value);
  void setTransition(const folly::dynamic &value);

 protected:
  void didUpdateProps() override;
  void layoutMembers() override;

 private:
  void load();
  void decode(const std::string &path, const std::string &cacheType, unsigned int request);
  void cancelDecode();
  void display();
  void fail(const std::string &message, unsigned int request);
  void fit();
  Evas_Coord_Size fittedSize() const;
  static void onPreloaded(void *data, Evas *evas, Evas_Object *object, void *info);
  void fadeIn();
  static Eina_Bool fadeStep(void *data, double position);

  Evas_Object *const image_;
  std::string uri_;
  std::string cacheKey_;
  std::string contentFit_{"cover"};
  std::string cacheType_;
  int imageWidth_{0};
  int imageHeight_{0};
  bool decoding_{false};
  double transitionSeconds_{0};
  bool sourceChanged_{false};
  unsigned int request_{0};
  Ecore_Animator *fade_{nullptr};
  std::shared_ptr<ExpoImageView *> alive_;
};

} // namespace rntizen::expo
