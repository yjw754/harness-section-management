#pragma once

#include <memory>
#include <vector>

#include "hrn/section/HrnPositionPointInput.h"
#include "hrn/section/HrnSectionDiagnostics.h"
#include "hrn/section/HrnSegmentSideRelation.h"

namespace hrn {
struct HrnCircularSolvedSide {
  std::vector<HrnSegment*> segments;
  std::vector<std::unique_ptr<HrnSectionData>> sections;
  std::unique_ptr<HrnSectionData> root;
  std::vector<Point2D> centers2d;
  std::vector<double> absolute_rotations;
  std::vector<Point3D> locations;
  size_t candidate_count = 0;
  size_t iterations_used = 0;
};
struct HrnCircularHierarchyResult {
  HrnSectionTransactionResult status;
  HrnCircularSolvedSide sides[2];
  const HrnCircularSolvedSide& side(ZeHrnSide value) const {
    return sides[value == ZeHrnSide::Positive ? 0 : 1];
  }
};
class HrnPositionPointSolver {
 public:
  HrnCircularHierarchyResult solveCircular(
      const HrnPositionPointInput& input,
      const std::vector<HrnSegmentSideRelation>& relations) const;
};
}  // namespace hrn
