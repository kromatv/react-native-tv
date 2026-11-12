#include "TvState.h"

namespace rntizen {

TvState::TvState(bool voiceConfigured, std::optional<std::string> searchUrl)
    : voiceConfigured_(voiceConfigured), searchUrl_(std::move(searchUrl)) {}

bool TvState::voiceConfigured() const {
  return voiceConfigured_;
}

const std::optional<std::string> &TvState::searchUrl() const {
  return searchUrl_;
}

bool TvState::voiceAvailable() const {
  return voiceAvailable_.load();
}

void TvState::setVoiceAvailable(bool available) {
  voiceAvailable_ = available;
}

bool TvState::listening(std::string_view event) const {
  return event == "search" ? searchListeners_.load() > 0 : commandListeners_.load() > 0;
}

void TvState::addListener(std::string_view event, int delta) {
  (event == "search" ? searchListeners_ : commandListeners_) += delta;
}

std::optional<std::string> TvState::screen() const {
  std::lock_guard lock(mutex_);
  return screen_;
}

void TvState::setScreen(std::optional<std::string> screen) {
  std::lock_guard lock(mutex_);
  screen_ = std::move(screen);
}

bool TvState::launchedFromTile() const {
  return launchedFromTile_.load();
}

void TvState::setLaunchedFromTile(bool fromTile) {
  launchedFromTile_ = fromTile;
}

TvState::PreviewPublisher TvState::previewPublisher() const {
  std::lock_guard lock(mutex_);
  return previewPublisher_;
}

void TvState::setPreviewPublisher(PreviewPublisher publisher) {
  std::lock_guard lock(mutex_);
  previewPublisher_ = std::move(publisher);
}

} // namespace rntizen
