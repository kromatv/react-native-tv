#include "ExpoViewRegistry.h"

#include "ExpoView.h"

#include "fabric/ComponentDescriptorRegistry.h"
#include "mounting/ComponentViewRegistry.h"

#include <ExpoViewComponentDescriptor.h>

#include <cctype>
#include <mutex>

namespace rntizen::expo {

namespace {

struct Registry {
  std::mutex mutex;
  std::map<std::string, std::shared_ptr<const ExpoViewType>> types;
};

Registry &registry() {
  static auto *shared = new Registry();
  return *shared;
}

facebook::react::ComponentDescriptor::Unique createDescriptor(
    const facebook::react::ComponentDescriptorParameters &parameters) {
  return std::make_unique<::expo::ExpoViewComponentDescriptor<>>(parameters);
}

std::string nativeEventName(const std::string &event) {
  if (event.rfind("on", 0) == 0) return "top" + event.substr(2);
  std::string name = event;
  if (!name.empty()) name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
  return "top" + name;
}

} // namespace

std::string expoViewComponentName(const std::string &moduleName, const std::string &viewName) {
  return viewName.empty() ? "ViewManagerAdapter_" + moduleName : "ViewManagerAdapter_" + moduleName + "_" + viewName;
}

bool registerExpoView(ExpoViewType type) {
  auto shared = std::make_shared<const ExpoViewType>(std::move(type));
  const auto componentName = expoViewComponentName(shared->moduleName, shared->viewName);
  auto flavor = std::make_shared<const std::string>(componentName);
  registerComponentDescriptor({
      .handle = reinterpret_cast<facebook::react::ComponentHandle>(flavor->c_str()),
      .name = flavor->c_str(),
      .flavor = flavor,
      .constructor = &createDescriptor,
  });
  registerComponentView(componentName, [shared](Evas *evas, facebook::react::Tag tag) -> std::unique_ptr<ComponentView> {
    auto view = shared->create(evas, tag);
    view->bindType(shared);
    return view;
  });
  auto &views = registry();
  std::lock_guard lock(views.mutex);
  views.types.insert_or_assign(componentName, std::move(shared));
  return true;
}

std::vector<std::shared_ptr<const ExpoViewType>> expoViewTypes(const std::string &moduleName) {
  std::vector<std::shared_ptr<const ExpoViewType>> types;
  auto &views = registry();
  std::lock_guard lock(views.mutex);
  for (const auto &[name, type] : views.types) {
    if (type->moduleName == moduleName) types.push_back(type);
  }
  return types;
}

std::optional<folly::dynamic> expoViewConfig(const std::string &moduleName, const std::string &viewName) {
  std::shared_ptr<const ExpoViewType> type;
  {
    auto &views = registry();
    std::lock_guard lock(views.mutex);
    auto found = views.types.find(expoViewComponentName(moduleName, viewName));
    if (found == views.types.end()) return std::nullopt;
    type = found->second;
  }
  folly::dynamic attributes = folly::dynamic::object();
  for (const auto &[name, setter] : type->props) attributes[name] = true;
  folly::dynamic events = folly::dynamic::object();
  for (const auto &event : type->events) {
    events[nativeEventName(event)] = folly::dynamic::object("registrationName", event);
  }
  return folly::dynamic::object("validAttributes", std::move(attributes))("directEventTypes", std::move(events));
}

} // namespace rntizen::expo
