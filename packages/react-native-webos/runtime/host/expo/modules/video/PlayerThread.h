#pragma once

#include <functional>

namespace rnwebos::expo {

/// Runs `task` on the one thread that creates, unprepares and destroys every
/// player, in submission order: a player released before another is created
/// is gone before the next one exists. capi-media-player lets these calls run
/// off the main loop (`webos-media-playback.md`, section 4).
void runOnPlayerThread(std::function<void()> task);

} // namespace rnwebos::expo
