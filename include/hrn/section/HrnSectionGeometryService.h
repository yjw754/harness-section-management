#pragma once

#include <vector>

#include "hrn/section/HrnGeometryTypes.h"

namespace hrn {
struct HrnCircle2D {
  Point2D center;
  double radius;
};

struct HrnContourDistanceResult {
  bool valid = false;
  double signed_distance = 0;
  Point2D lhs_witness{0, 0};
  Point2D rhs_witness{0, 0};
};

struct HrnContainmentResult {
  bool valid = false;
  bool contained = false;
  double minimum_clearance = 0;
  Point2D parent_witness{0, 0};
  Point2D child_witness{0, 0};
};

struct HrnMinimumEnclosingCircleResult {
  bool valid = false;
  HrnCircle2D circle{{0, 0}, 0};
};

class HrnCircularGeometryService {
 public:
  HrnContourDistanceResult signedDistance(const HrnCircle2D& lhs, const HrnCircle2D& rhs,
                                          double modeling_tolerance) const;
  HrnContainmentResult containsWithClearance(const HrnCircle2D& parent,
                                             const HrnCircle2D& child,
                                             double required_clearance,
                                             double modeling_tolerance) const;
  bool intersects(const HrnCircle2D& lhs, const HrnCircle2D& rhs,
                  double modeling_tolerance) const;
  HrnMinimumEnclosingCircleResult minimumEnclosingCircle(
      const std::vector<HrnCircle2D>& circles, double modeling_tolerance) const;
};
}  // namespace hrn
