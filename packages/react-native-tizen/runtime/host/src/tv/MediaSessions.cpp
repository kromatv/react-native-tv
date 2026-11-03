#include "MediaSessions.h"

#include "../logging/Log.h"

#include <algorithm>
#include <mutex>
#include <vector>

namespace rntizen {

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
  const auto playing = std::find_if(live.begin(), live.end(), [](const auto &session) { return session->isPlaying(); });
  if (playing != live.end()) return *playing;
  const auto active =
      std::find_if(live.begin(), live.end(), [&](const auto &session) { return session.get() == sessions.active; });
  if (active != live.end()) return *active;
  return live.empty() ? nullptr : live.back();
}

void pauseMediaSessionsForBackground(bool mediaCategoryDeclared) {
  int paused = 0;
  int kept = 0;
  for (const auto &session : snapshot().live) {
    if (!session->isPlaying()) continue;
    if (session->staysActiveInBackground()) {
      ++kept;
    } else {
      session->pause();
      ++paused;
    }
  }
  if (paused > 0) log(LogLevel::Info, "Background: paused " + std::to_string(paused) + " player(s)");
  if (kept == 0) return;
  log(mediaCategoryDeclared ? LogLevel::Info : LogLevel::Warning,
      "Background: " + std::to_string(kept) + " player(s) stay active" +
          (mediaCategoryDeclared ? ""
                                 : ", but the manifest has no media background category, so the TV suspends the app; "
                                   "set supportsBackgroundPlayback in expo-video's config plugin"));
}

} // namespace rntizen
