#pragma once

#include <functional>

namespace rntizen::expo {

/// Runs `task` on the one thread that creates, unprepares and destroys every
/// player, in submission order: a player released before another is created
/// is gone before the next one exists. capi-media-player lets these calls run
/// off the main loop (`tizen-media-playback.md`, section 4).
void runOnPlayerThread(std::function<void()> task);

} // namespace rntizen::expo
