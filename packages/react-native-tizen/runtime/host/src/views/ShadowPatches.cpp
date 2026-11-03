#include "ShadowPatches.h"

#include "../threading/MainThread.h"
#include "../threading/TaskQueueThread.h"

#include <array>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace rntizen {

namespace {

constexpr std::size_t kCacheLimit = 256;

using Key = std::array<int, 9>;
using Ready = std::function<void(const NinePatch &)>;

struct Store {
  std::map<Key, NinePatch> patches;
  std::map<Key, std::vector<Ready>> waiting;
};

Store &store() {
  static Store shared;
  return shared;
}

TaskQueueThread &painter() {
  static auto *thread = new TaskQueueThread("rn-tizen-patches");
  return *thread;
}

Key keyFor(const Corners &c, int blur) {
  return {c.topLeft.x, c.topLeft.y, c.topRight.x, c.topRight.y, c.bottomRight.x, c.bottomRight.y, c.bottomLeft.x,
          c.bottomLeft.y, blur};
}

void deliver(const Key &key, NinePatch patch) {
  auto &shared = store();
  if (shared.patches.size() >= kCacheLimit) shared.patches.clear();
  const auto &stored = shared.patches.insert_or_assign(key, std::move(patch)).first->second;
  auto found = shared.waiting.find(key);
  if (found == shared.waiting.end()) return;
  auto callbacks = std::move(found->second);
  shared.waiting.erase(found);
  for (const auto &ready : callbacks) ready(stored);
}

} // namespace

const NinePatch *cachedShadowPatch(const Corners &corners, int blur) {
  const auto &patches = store().patches;
  const auto found = patches.find(keyFor(corners, blur));
  return found == patches.end() ? nullptr : &found->second;
}

void requestShadowPatch(const Corners &corners, int blur, Ready ready) {
  const auto key = keyFor(corners, blur);
  if (const auto *patch = cachedShadowPatch(corners, blur)) {
    ready(*patch);
    return;
  }
  auto &queue = store().waiting[key];
  queue.push_back(std::move(ready));
  if (queue.size() > 1) return;
  painter().runOnQueue([key, corners, blur] {
    auto patch = std::make_shared<NinePatch>(makeShadowPatch(corners, blur));
    runOnMainThread([key, patch] { deliver(key, std::move(*patch)); });
  });
}

} // namespace rntizen
