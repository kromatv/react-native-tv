#include "PipelineClaims.h"

#include <doctest/doctest.h>

#include <string>
#include <utility>
#include <vector>

namespace rnwebos::expo {

namespace {

using Lines = std::vector<std::string>;

class Player final : public PipelineClaimant {
 public:
  Player(std::string name, Lines &lines) : name_(std::move(name)), lines_(lines) {}

  void pipelineGranted() override {
    lines_.push_back(name_ + " granted");
  }
  void pipelineTaken() override {
    lines_.push_back(name_ + " taken");
  }

 private:
  const std::string name_;
  Lines &lines_;
};

struct Players {
  Lines lines;
  PipelineClaims claims;
  Player first{"first", lines};
  Player second{"second", lines};
  Player third{"third", lines};

  Lines take() {
    return std::exchange(lines, {});
  }
};

} // namespace

TEST_CASE("the first to ask has the pipeline, and among players no view shows the last to ask takes it") {
  Players players;
  auto &[lines, claims, first, second, third] = players;
  claims.claim(first, false);
  CHECK(players.take() == Lines{"first granted"});
  CHECK(claims.holds(first));
  claims.claim(second, false);
  CHECK(players.take() == Lines{"first taken", "second granted"});
  CHECK_FALSE(claims.holds(first));
  claims.claim(first, false);
  CHECK(players.take() == Lines{"second taken", "first granted"});
}

TEST_CASE("a player a view shows keeps the pipeline from one no view shows, until that one is shown too") {
  Players players;
  auto &[lines, claims, first, second, third] = players;
  claims.claim(first, true);
  claims.claim(second, false);
  CHECK(players.take() == Lines{"first granted"});
  claims.setShown(second, false);
  claims.setShown(third, true);
  CHECK(lines.empty());
  claims.setShown(second, true);
  CHECK(players.take() == Lines{"first taken", "second granted"});
  claims.setShown(second, true);
  claims.setShown(first, true);
  CHECK(lines.empty());
}

TEST_CASE("a view that lets go of the holder passes the pipeline to a newer player that waits") {
  Players players;
  auto &[lines, claims, first, second, third] = players;
  claims.claim(first, true);
  claims.claim(second, false);
  lines.clear();
  claims.setShown(first, false);
  CHECK(players.take() == Lines{"first taken", "second granted"});
  claims.setShown(first, true);
  CHECK(players.take() == Lines{"second taken", "first granted"});
}

TEST_CASE("a holder that drops its claim passes the pipeline on, shown players first, and is told nothing") {
  Players players;
  auto &[lines, claims, first, second, third] = players;
  claims.claim(first, true);
  claims.claim(second, true);
  claims.claim(third, false);
  CHECK(players.take() == Lines{"first granted", "first taken", "second granted"});
  claims.drop(second);
  CHECK(players.take() == Lines{"first granted"});
  claims.drop(third);
  claims.drop(third);
  CHECK(lines.empty());
  claims.drop(first);
  CHECK(lines.empty());
  CHECK_FALSE(claims.holds(first));
}

TEST_CASE("a forgotten holder is told nothing and neither is the next in line, until someone asks") {
  Players players;
  auto &[lines, claims, first, second, third] = players;
  claims.claim(first, true);
  claims.claim(second, false);
  lines.clear();
  claims.forget(first);
  CHECK(lines.empty());
  CHECK_FALSE(claims.holds(second));
  claims.claim(third, false);
  CHECK(players.take() == Lines{"third granted"});
}

} // namespace rnwebos::expo
