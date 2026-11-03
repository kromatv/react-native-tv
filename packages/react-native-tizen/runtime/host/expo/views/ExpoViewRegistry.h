#pragma once

#include <folly/dynamic.h>
#include <react/renderer/core/ReactPrimitives.h>

#include <Evas.h>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace rntizen::expo {

class ExpoView;

/// Applies one prop JavaScript sent to a view; `value` is null once the prop is removed.
using ExpoPropSetter = std::function<void(ExpoView &view, const folly::dynamic &value)>;

/// A method JavaScript calls on a mounted view through its ref, on the main thread.
using ExpoViewFunction = std::function<void(ExpoView &view)>;

/// One native view an Expo module defines. The same description answers
/// `expo.getViewConfig`, gives Fabric its component descriptor and tells the
/// host which view to mount, so the three cannot disagree. `viewName` is empty
/// for the module's default view, and `events` are named as JavaScript names
/// them, such as `onLoad`. `functions` become the view's methods, which
/// expo-modules-core reads from the module's `ViewPrototypes`.
struct ExpoViewType {
  std::string moduleName;
  std::string viewName;
  std::map<std::string, ExpoPropSetter, std::less<>> props;
  std::vector<std::string> events;
  std::map<std::string, ExpoViewFunction, std::less<>> functions;
  std::function<std::unique_ptr<ExpoView>(Evas *evas, facebook::react::Tag tag)> create;
};

/// The component name expo-modules-core gives a view:
/// `ViewManagerAdapter_<module>`, or `ViewManagerAdapter_<module>_<view>`.
std::string expoViewComponentName(const std::string &moduleName, const std::string &viewName);

/// Registers the view's Fabric component descriptor, its native view and its
/// view config. Call while the library loads, with RN_TIZEN_EXPO_VIEW.
bool registerExpoView(ExpoViewType type);

/// Every view the module `moduleName` defines.
std::vector<std::shared_ptr<const ExpoViewType>> expoViewTypes(std::string_view moduleName);

/// What `expo.getViewConfig(module, view)` returns: `{ validAttributes,
/// directEventTypes }`, or nullopt for a view no module defines.
std::optional<folly::dynamic> expoViewConfig(const std::string &moduleName, const std::string &viewName);

} // namespace rntizen::expo
