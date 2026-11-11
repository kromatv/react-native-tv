#pragma once

#include "ExpoView.h"
#include "ExpoViewRegistry.h"

#include "registry/Registration.h"

#include <functional>
#include <initializer_list>
#include <string>

namespace rnwebos::expo {

/// Describes an Expo view, `View`, an ExpoView constructed from (Evas *, Tag),
/// in the shape of Expo's `View` DSL:
///
///   RN_WEBOS_EXPO_VIEW(ViewDefinition<ImageView>("ExpoImage")
///                          .prop("source", &ImageView::setSource)
///                          .events({"onLoad", "onError"}));
template <typename View>
class ViewDefinition {
  static_assert(std::is_base_of_v<ExpoView, View>, "An Expo view derives from ExpoView");

 public:
  explicit ViewDefinition(std::string moduleName, std::string viewName = {}) {
    type_.moduleName = std::move(moduleName);
    type_.viewName = std::move(viewName);
    type_.create = [](Evas *evas, facebook::react::Tag tag) { return std::make_unique<View>(evas, tag); };
  }

  template <typename Setter>
  ViewDefinition &prop(const char *name, Setter setter) {
    type_.props.insert_or_assign(name, [setter](ExpoView &view, const folly::dynamic &value) {
      std::invoke(setter, static_cast<View &>(view), value);
    });
    return *this;
  }

  /// A method JavaScript calls through the view's ref: `method()` runs on the
  /// main thread and the promise JavaScript gets resolves after it.
  template <typename Method>
  ViewDefinition &asyncFunction(const char *name, Method method) {
    type_.functions.insert_or_assign(name, [method](ExpoView &view) { std::invoke(method, static_cast<View &>(view)); });
    return *this;
  }

  ViewDefinition &events(std::initializer_list<const char *> names) {
    for (const auto *name : names) type_.events.emplace_back(name);
    return *this;
  }

  explicit operator ExpoViewType() const {
    return type_;
  }

 private:
  ExpoViewType type_;
};

} // namespace rnwebos::expo

/// Registers an Expo view from a ViewDefinition when the library loads.
#define RN_WEBOS_EXPO_VIEW(definition) \
  RN_WEBOS_REGISTRATION(::rnwebos::expo::registerExpoView(static_cast<::rnwebos::expo::ExpoViewType>(definition)))
