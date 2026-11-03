#include "PlayerThread.h"

#include "threading/TaskQueueThread.h"

namespace rntizen::expo {

void runOnPlayerThread(std::function<void()> task) {
  static auto *thread = new TaskQueueThread("rn-tizen-player");
  thread->runOnQueue(std::move(task));
}

} // namespace rntizen::expo
