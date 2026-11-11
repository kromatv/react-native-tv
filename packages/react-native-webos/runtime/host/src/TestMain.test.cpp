#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include "platform/MainLoop.h"

int main(int argc, char **argv) {
  // The thread that first asks for the loop is the main thread. A test's
  // network thread must not get there before the thread the tests run on.
  rnwebos::MainLoop::get();
  return doctest::Context(argc, argv).run();
}
