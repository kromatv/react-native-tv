#include "MediaSessions.h"

#include "../logging/Log.h"

#include <algorithm>
#include <format>
#include <mutex>
#include <vector>

namespace rnwebos {

namespace {

struct Registry {
  std::mutex mutex;
  std::vector<std::weak_ptr<MediaSession>> sessions;
  const MediaSession *active{nullptr};
};

Registry &registry() {
  static Registry instance;
  return instance;
}

struct Snapshot {
  std::vector<std::shared_ptr<MediaSession>> live;
  const MediaSession *active{nullptr};
};

Snapshot snapshot() {
  auto &state = registry();
  std::lock_guard lock(state.mutex);
  std::erase_if(state.sessions, [](const std::weak_ptr<MediaSession> &weak) { return weak.expired(); });
  Snapshot result{.live = {}, .active = state.active};
  for (const auto &weak : state.sessions) {
    if (auto session = weak.lock()) result.live.push_back(std::move(session));
  }
  return result;
}

} // namespace

MediaSession::~MediaSession() = default;

void MediaSession::appForegroundChanged(bool) {
  // A session without a pipeline has nobody to tell.
}

void addMediaSession(const std::shared_ptr<MediaSession> &session) {
  auto &state = registry();
  std::lock_guard lock(state.mutex);
  state.sessions.push_back(session);
}

void markMediaSessionActive(const MediaSession *session) {
  auto &state = registry();
  std::lock_guard lock(state.mutex);
  state.active = session;
}

std::shared_ptr<MediaSession> currentMediaSession() {
  const auto sessions = snapshot();
  const auto &live = sessions.live;
  const auto isPlaying = [](const auto &session) { return session->isPlaying(); };
  if (const auto playing = std::ranges::find_if(live, isPlaying); playing != live.end()) return *playing;
  const auto isActive = [&sessions](const auto &session) { return session.get() == sessions.active; };
  if (const auto active = std::ranges::find_if(live, isActive); active != live.end()) return *active;
  return live.empty() ? nullptr : live.back();
}

void pauseMediaSessionsForBackground(bool backgroundMediaDeclared) {
  int paused = 0;
  int kept = 0;
  for (const auto &session : snapshot().live) {
    session->appForegroundChanged(false);
    if (!session->isPlaying()) continue;
    if (session->staysActiveInBackground()) {
      ++kept;
    } else {
      session->pause();
      ++paused;
    }
  }
  if (paused > 0) log(LogLevel::Info, std::format("Background: paused {} player(s)", paused));
  if (kept == 0) return;
  if (backgroundMediaDeclared) {
    log(LogLevel::Info, std::format("Background: {} player(s) stay active", kept));
    return;
  }
  log(LogLevel::Warning,
      std::format(
          "Background: {} player(s) stay active, but the app's config does not set backgroundMedia, so the TV may "
          "stop them",
          kept));
}

void resumeMediaSessionsForForeground() {
  for (const auto &session : snapshot().live) session->appForegroundChanged(true);
}

} // namespace rnwebos
