#pragma once

#include <jsi/jsi.h>

#include <memory>
#include <string>

namespace rntizen::expo {

class Context;

/// Sets `ViewPrototypes` on the module object of `moduleName` when the module
/// defines views: one object per view, keyed `<module>_<view>` (or `<module>`
/// for its default view), holding the view's functions. expo-modules-core
/// copies them onto the component, and each finds its view by the
/// component's `nativeTag`. JavaScript thread only.
void installViewPrototypes(
    facebook::jsi::Runtime &runtime,
    const std::shared_ptr<Context> &context,
    const facebook::jsi::Object &moduleObject,
    const std::string &moduleName);

} // namespace rntizen::expo
