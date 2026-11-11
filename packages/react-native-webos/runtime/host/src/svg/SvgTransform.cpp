#include "SvgTransform.h"

namespace rnwebos::svg {

SkMatrix matrixFromProp(const std::vector<facebook::react::Float> &matrix) {
  if (matrix.size() != 6) return SkMatrix::I();
  return SkMatrix::MakeAll(matrix[0], matrix[2], matrix[4], matrix[1], matrix[3], matrix[5], 0, 0, 1);
}

} // namespace rnwebos::svg
