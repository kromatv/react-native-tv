#pragma once

#include "platform/LunaConnection.h"

#include <functional>
#include <memory>
#include <string>

namespace rnwebos::expo {

/// Where a media's subscription delivers its events: replaced when the media
/// is handed over to be unloaded.
using LunaEventSink = std::shared_ptr<std::function<void(const folly::dynamic &)>>;

/// Unloads `mediaId` and holds every later load back until the pipeline says
/// it is gone, or a short while has passed: the pipeline frees its decoder
/// only then (docs/reference/webos-media-playback.md, "Play this HLS URL",
/// step 9). `events` is the media's subscription and `sink` where it
/// delivers; both, and the `plane` the media draws on, are kept until that
/// point. Main thread only.
void unloadMedia(
    LunaConnection &bus,
    const std::string &mediaId,
    std::unique_ptr<LunaConnection::Subscription> events,
    LunaEventSink sink,
    std::shared_ptr<void> plane,
    int player);

/// Runs `task` once no unload is under way: now, when there is none.
void afterUnloads(std::function<void()> task);

} // namespace rnwebos::expo
