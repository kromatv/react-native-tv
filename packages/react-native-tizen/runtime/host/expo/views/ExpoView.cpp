#include "ExpoView.h"

#include <ExpoViewEventEmitter.h>
#include <ExpoViewProps.h>
#include <jsi/JSIDynamic.h>

#include <unordered_map>

namespace rntizen::expo {

namespace {

std::unordered_map<facebook::react::Tag, ExpoView *> &mountedViews() {
  static std::unordered_map<facebook::react::Tag, ExpoView *> views;
  return views;
}

} // namespace

ExpoView::ExpoView(Evas *evas, facebook::react::Tag tag) : ComponentView(evas, tag) {
  mountedViews().insert_or_assign(tag, this);
}

ExpoView::~ExpoView() {
  auto found = mountedViews().find(tag());
  if (found != mountedViews().end() && found->second == this) mountedViews().erase(found);
}

ExpoView *ExpoView::find(facebook::react::Tag tag) {
  auto found = mountedViews().find(tag);
  return found == mountedViews().end() ? nullptr : found->second;
}

void ExpoView::bindType(std::shared_ptr<const ExpoViewType> type) {
  type_ = std::move(type);
}

void ExpoView::updateProps(const facebook::react::Props::Shared &props) {
  ComponentView::updateProps(props);
  if (!type_) return;
  const auto &expoProps = static_cast<const ::expo::ExpoViewProps &>(*props);
  for (const auto &[name, setter] : type_->props) {
    auto found = expoProps.propsMap.find(name);
    const folly::dynamic value = found == expoProps.propsMap.end() ? folly::dynamic(nullptr) : found->second;
    auto previous = applied_.find(name);
    if (previous == applied_.end() ? value.isNull() : previous->second == value) continue;
    applied_.insert_or_assign(name, value);
    setter(*this, value);
  }
  didUpdateProps();
}

void ExpoView::dispatchEvent(const std::string &event, folly::dynamic payload) const {
  auto emitter = std::dynamic_pointer_cast<const ::expo::ExpoViewEventEmitter>(eventEmitter());
  if (!emitter) return;
  emitter->dispatch(event, [payload = std::move(payload)](facebook::jsi::Runtime &runtime) {
    return facebook::jsi::valueFromDynamic(runtime, payload);
  });
}

} // namespace rntizen::expo
