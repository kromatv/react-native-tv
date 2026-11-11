#pragma once

#include "SvgGradient.h"
#include "SvgNode.h"

namespace rnwebos::svg {

/// `<LinearGradient>` or `<RadialGradient>`: draws nothing in place, and is
/// found by its name when a shape paints with `url(#id)`.
class GradientNode final : public Node {
 public:
  GradientDefinition definition;

  bool definesOnly() const override;
};

} // namespace rnwebos::svg
