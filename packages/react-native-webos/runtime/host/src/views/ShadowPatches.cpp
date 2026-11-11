#include "ShadowPatches.h"

#include "../threading/MainThread.h"
#include "../threading/TaskQueueThread.h"
#include "PatchCache.h"

#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace rntizen {

namespace {

using Ready = std::function<void(const NinePatch &)>;

std::map<PatchKey, std::vector<Ready>> &waiting() {
  static std::map<PatchKey, std::vector<Ready>> requests;
  return requests;
}

TaskQueueThread &painter() {
  static auto *thread = new TaskQueueThread("rn-tizen-patches");
  return *thread;
}

PatchKey shadowKey(const Corners &corners, int blur) {
  auto key = patchKey(PatchKind::Shadow, corners);
  key.push_back(static_cast<float>(blur));
  return key;
}

void deliver(const PatchKey &key, NinePatch patch) {
  const auto &stored = patchCache().insert(key, std::move(patch));
  auto &requests = waiting();
  auto found = requests.find(key);
  if (found == requests.end()) return;
  auto callbacks = std::move(found->second);
  requests.erase(found);
  for (const auto &ready : callbacks) ready(stored);
}

} // namespace

const NinePatch *cachedShadowPatch(const Corners &corners, int blur) {
  return patchCache().find(shadowKey(corners, blur));
}

void requestShadowPatch(const Corners &corners, int blur, Ready ready) {
  auto key = shadowKey(corners, blur);
  if (const auto *patch = patchCache().find(key)) {
    ready(*patch);
    return;
  }
  auto &queue = waiting()[key];
  queue.push_back(std::move(ready));
  if (queue.size() > 1) return;
  painter().runOnQueue([key = std::move(key), corners, blur] {
    auto patch = std::make_shared<NinePatch>(makeShadowPatch(corners, blur));
    runOnMainThread([key, patch] { deliver(key, std::move(*patch)); });
  });
}

} // namespace rntizen
