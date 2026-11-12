#include "MicButton.h"

#include <utility>

namespace rntizen {

MicButton::MicButton(Emit emit, Post post) : emit_(std::move(emit)), post_(std::move(post)) {}

void MicButton::setGrab(Grab grab) {
  grab_ = std::move(grab);
  grabbed_ = false;
  apply();
}

void MicButton::claim() {
  ++claims_;
  applyLater();
}

void MicButton::unclaim() {
  if (--claims_ < 0) claims_ = 0;
  applyLater();
}

bool MicButton::handleKey(std::string_view keyName, bool isDown) {
  if (keyName != kMicButtonKey || !grabbed_) return false;
  if (isDown == held_) return true;
  held_ = isDown;
  emit_(isDown ? "pressIn" : "pressOut");
  return true;
}

void MicButton::releaseHeld() {
  if (!held_) return;
  held_ = false;
  emit_("pressOut");
}

void MicButton::reset() {
  claims_ = 0;
  held_ = false;
  apply();
}

void MicButton::applyLater() {
  post_([weak = weak_from_this()] {
    if (auto button = weak.lock()) button->apply();
  });
}

void MicButton::apply() {
  const bool wanted = claims_ > 0;
  if (!grab_ || wanted == grabbed_) return;
  grab_(wanted);
  grabbed_ = wanted;
  // A key released after its grab is gone never reaches the app (Tizen NUI's
  // GrabKey, tizen-input.md section 4), so the press ends here.
  if (!wanted) releaseHeld();
}

} // namespace rntizen
