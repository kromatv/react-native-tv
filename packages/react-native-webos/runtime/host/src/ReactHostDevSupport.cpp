#include "ReactHost.h"

#if RN_TIZEN_DEV_SUPPORT
#include "../devsupport/DevSupport.h"
#endif

namespace rntizen {

namespace {

constexpr const char *kLogBoxAppKey = "LogBox";

} // namespace

void ReactHost::startDevSupport() {
#if RN_TIZEN_DEV_SUPPORT
  if (!config_.bundleUrl) return;
  devSupport_ = DevSupport::start(
      *config_.bundleUrl,
      device_,
      DevSupport::Host{
          .evas = evas_object_evas_get(window_),
          .screenWidth = device_.screenWidth,
          .screenHeight = device_.screenHeight,
          .reload = [this] { reload(); },
          .showLogBox = [this](bool shown) { showLogBox(shown); },
      });
#endif
}

void ReactHost::devSupportInstanceStarted() {
#if RN_TIZEN_DEV_SUPPORT
  if (devSupport_ != nullptr) devSupport_->instanceStarted(instanceGate_, instance_->getBufferedRuntimeExecutor());
#endif
}

facebook::react::jsinspector_modern::HostTarget *ReactHost::inspectorTarget() {
#if RN_TIZEN_DEV_SUPPORT
  if (devSupport_ != nullptr) return devSupport_->inspectorTarget();
#endif
  return nullptr;
}

void ReactHost::showLogBox(bool shown) {
  if (presenter_ == nullptr) return;
  if (!shown) {
    presenter_->stopOverlaySurface(kLogBoxAppKey);
    return;
  }
  presenter_->startOverlaySurface(
      kLogBoxAppKey,
      {.width = static_cast<facebook::react::Float>(device_.screenWidth),
       .height = static_cast<facebook::react::Float>(device_.screenHeight)});
}

} // namespace rntizen
