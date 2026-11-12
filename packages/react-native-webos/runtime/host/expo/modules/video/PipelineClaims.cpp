#include "PipelineClaims.h"

#include <algorithm>
#include <utility>

namespace rnwebos::expo {

PipelineClaims &PipelineClaims::shared() {
  static PipelineClaims instance;
  return instance;
}

std::vector<PipelineClaims::Claim>::iterator PipelineClaims::find(const PipelineClaimant &claimant) {
  return std::ranges::find(claims_, &claimant, &Claim::claimant);
}

void PipelineClaims::claim(PipelineClaimant &claimant, bool shown) {
  if (const auto found = find(claimant); found != claims_.end()) {
    found->shown = shown;
    found->order = ++order_;
  } else {
    claims_.push_back({.claimant = &claimant, .shown = shown, .order = ++order_});
  }
  settle();
}

void PipelineClaims::setShown(const PipelineClaimant &claimant, bool shown) {
  const auto found = find(claimant);
  if (found == claims_.end() || found->shown == shown) return;
  found->shown = shown;
  if (shown) found->order = ++order_;
  settle();
}

bool PipelineClaims::holds(const PipelineClaimant &claimant) const {
  return holder_ == &claimant;
}

void PipelineClaims::drop(const PipelineClaimant &claimant) {
  forget(claimant);
  settle();
}

void PipelineClaims::forget(const PipelineClaimant &claimant) {
  if (const auto found = find(claimant); found != claims_.end()) claims_.erase(found);
  if (holder_ == &claimant) holder_ = nullptr;
}

void PipelineClaims::settle() {
  const auto first = std::ranges::max_element(claims_, {}, [](const Claim &claim) {
    return std::pair(claim.shown, claim.order);
  });
  auto *winner = first == claims_.end() ? nullptr : first->claimant;
  if (winner == holder_) return;
  if (auto *loser = std::exchange(holder_, winner); loser != nullptr) loser->pipelineTaken();
  if (winner != nullptr) winner->pipelineGranted();
}

} // namespace rnwebos::expo
