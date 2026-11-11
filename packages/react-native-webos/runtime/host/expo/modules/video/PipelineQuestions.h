#pragma once

#include "PlayerBackend.h"

#include <folly/dynamic.h>

#include <chrono>
#include <string>
#include <vector>

/// Every place where docs/reference/webos-media-playback.md has an open
/// question, or where the projects it was written from disagree: the answer
/// the player goes with, all in this one file. Playback logs each of them at
/// debug level when it meets it, as `[video 2] open question: ...`, so a
/// session on a TV settles them from the log and the fix is one function here.
namespace rnwebos::expo::pipeline {

/// Logs that the player numbered `player` met the open question `what`.
void openQuestion(int player, const std::string &what);

/// The names to register on the Luna bus as, first to last, for the app `appId`.
std::vector<std::string> busNames(const std::string &appId, int processId);

/// The payload of `luna://com.webos.media/load`.
folly::dynamic loadPayload(const MediaSource &source, const std::string &appId, const std::string &windowId, int player);

folly::dynamic subscribePayload(const std::string &mediaId);

folly::dynamic playRatePayload(const std::string &mediaId, double rate);

folly::dynamic volumePayload(const std::string &mediaId, double volume);

/// The end of the buffered range in seconds, from a `bufferRange` event's `endTime`.
double bufferedSeconds(double endTime);

/// Whether an `error` event ends playback. The others are logged and playback goes on.
bool endsPlayback(int errorCode);

/// How long the pipeline has to confirm a call with its event before the
/// player goes on as if it had: `seekDone`, `playing`, `paused` and
/// `trackSelected` do not arrive on every release.
inline constexpr std::chrono::milliseconds kSeekDoneWait{3000};
inline constexpr std::chrono::milliseconds kPlayStateWait{1500};
inline constexpr std::chrono::milliseconds kTrackSelectedWait{1000};

/// How long a load waits for the `unloadCompleted` of the media before it.
inline constexpr std::chrono::milliseconds kUnloadWait{1500};

} // namespace rnwebos::expo::pipeline
