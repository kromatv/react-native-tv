#include "PlayerThread.h"

#include "threading/TaskQueueThread.h"

#include <folly/Indestructible.h>

namespace rntizen::expo {

void runOnPlayerThread(std::function<void()> task) {
  static folly::Indestructible<TaskQueueThread> thread("rn-tizen-player");
  thread->runOnQueue(std::move(task));
}

} // namespace rntizen::expo
