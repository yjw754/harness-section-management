#pragma once

#include <memory>
#include <vector>

#include "hrn/section/HrnPositionPointInput.h"
#include "hrn/section/HrnSectionDiagnostics.h"
#include "hrn/section/HrnSegmentSideRelation.h"

namespace hrn {
constexpr double kHrnCircularModelingTolerance = 1e-9;
class HrnCircularPlacementSnapshot;

struct HrnCircularSolvedSide {
  std::vector<HrnSegment*> segments;
  std::vector<std::unique_ptr<HrnSectionData>> sections;
  std::unique_ptr<HrnSectionData> root;
  std::vector<Point2D> centers2d;
  std::vector<Point2D> reference_centers;
  std::vector<double> absolute_rotations;
  std::vector<Point3D> locations;
  double reference_root_rotation = 0;
  size_t candidate_count = 0;
  size_t iterations_used = 0;
};
struct HrnCircularHierarchyResult {
  HrnSectionTransactionResult status;
  HrnCircularSolvedSide sides[2];
  bool used_global_rotation = false;
  double frame_rotation = 0;
  std::shared_ptr<const HrnCircularPlacementSnapshot> placement;
  const HrnCircularSolvedSide& side(ZeHrnSide value) const {
    return sides[value == ZeHrnSide::Positive ? 0 : 1];
  }
};
struct HrnCircularRotationBaseline {
  struct Side {
    std::vector<Point2D> centers;
    std::vector<double> absolute_rotations;
    std::vector<double> input_rotations;
    double root_rotation = 0;
    double reference_root_rotation = 0;
  } sides[2];
  bool prefer_negative = false;
  double fallback_delta = 0;
  double frame_rotation = 0;
  std::shared_ptr<const HrnCircularPlacementSnapshot> placement;
};
class HrnPositionPointSolver {
 public:
  HrnCircularHierarchyResult solveCircular(
      const HrnPositionPointInput& input,
      const std::vector<HrnSegmentSideRelation>& relations,
      const HrnCircularRotationBaseline* rotation = nullptr) const;
};
}  // namespace hrn
