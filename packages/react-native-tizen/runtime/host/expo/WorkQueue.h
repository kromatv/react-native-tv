#pragma once

#include <functional>

namespace rntizen::expo {

/// Runs `task` on one of a few background threads shared by every Expo module
/// and view, the way Expo runs an AsyncFunction off the JavaScript thread. A
/// task that throws is logged.
void runInBackground(std::function<void()> task);

} // namespace rntizen::expo
