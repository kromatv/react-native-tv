#pragma once

#include "ExpoViewRegistry.h"

#include "views/ComponentView.h"

#include <folly/dynamic.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace rntizen::expo {

/// The native side of an Expo view: a ComponentView that receives the props
/// its ExpoViewType declares, one setter call per changed prop, and sends
/// events to the JavaScript props named after them. Main thread only.
class ExpoView : public ComponentView {
 public:
  ExpoView(Evas *evas, facebook::react::Tag tag);
  ~ExpoView() override;

  /// The mounted Expo view with `tag`, or null.
  static ExpoView *find(facebook::react::Tag tag);

  void updateProps(const facebook::react::Props::Shared &props) override;

  void bindType(std::shared_ptr<const ExpoViewType> type);

 protected:
  /// Runs after every props update, once each changed prop was applied.
  virtual void didUpdateProps() {}

  /// Calls the JavaScript prop `event` (such as `onLoad`) with `payload` as its `nativeEvent`.
  void dispatchEvent(const std::string &event, folly::dynamic payload) const;

 private:
  std::shared_ptr<const ExpoViewType> type_;
  std::unordered_map<std::string, folly::dynamic> applied_;
};

} // namespace rntizen::expo
