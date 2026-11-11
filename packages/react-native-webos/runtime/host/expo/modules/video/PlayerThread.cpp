#include "PlayerThread.h"

#include "threading/TaskQueueThread.h"

#include <folly/Indestructible.h>

namespace rnwebos::expo {

void runOnPlayerThread(std::function<void()> task) {
  static folly::Indestructible<TaskQueueThread> thread("rn-webos-player");
  thread->runOnQueue(std::move(task));
}

} // namespace rnwebos::expo
