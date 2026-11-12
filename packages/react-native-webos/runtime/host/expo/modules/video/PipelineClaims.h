#pragma once

#include <vector>

namespace rnwebos::expo {

/// A player that wants the TV's media pipeline.
class PipelineClaimant {
 public:
  virtual ~PipelineClaimant() = default;
  /// The pipeline is this player's from now on.
  virtual void pipelineGranted() = 0;
  /// Another player takes the pipeline: this one unloads, and waits for it.
  virtual void pipelineTaken() = 0;
};

/// Which player holds the TV's media pipeline. A video loaded while another
/// was still loaded played without a picture on a TV, so one player loads at a
/// time (docs/reference/webos-media-playback.md, section 5): the one a view
/// shows, the last shown first, and among those no view shows the one that
/// asked last. The others wait for their turn. Main thread only.
class PipelineClaims {
 public:
  static PipelineClaims &shared();

  /// `claimant` wants the pipeline until it drops the claim. Asking again moves it to the front of its kind.
  void claim(PipelineClaimant &claimant, bool shown);
  void setShown(const PipelineClaimant &claimant, bool shown);
  bool holds(const PipelineClaimant &claimant) const;
  /// `claimant` wants the pipeline no longer, and the next in line gets it.
  void drop(const PipelineClaimant &claimant);
  /// Takes `claimant` out without telling anyone: it is being destroyed.
  void forget(const PipelineClaimant &claimant);

 private:
  struct Claim {
    PipelineClaimant *claimant{nullptr};
    bool shown{false};
    unsigned long order{0};
  };

  std::vector<Claim>::iterator find(const PipelineClaimant &claimant);
  void settle();

  std::vector<Claim> claims_;
  PipelineClaimant *holder_{nullptr};
  unsigned long order_{0};
};

} // namespace rnwebos::expo
