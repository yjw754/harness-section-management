#pragma once

#include <vector>

#include "hrn/section/HrnSectionDiagnostics.h"
#include "hrn/section/HrnSectionLayout.h"

namespace hrn {
struct HrnCircularLayoutMember {
  size_t id;
  double radius;
};
struct HrnCircularLayoutRequest {
  std::vector<HrnCircularLayoutMember> members;
  double spacing = 0;
  HrnLayoutSolveSettings settings;
};
struct HrnCircularLayoutPlacement {
  size_t id;
  double radius;
  Point2D center;
};
struct HrnRuleLayoutResult {
  bool succeeded = false;
  HrnSectionFailureReason reason = HrnSectionFailureReason::None;
  std::vector<HrnCircularLayoutPlacement> placements;
  double required_parent_radius = 0;
  size_t candidate_count = 0;
  size_t iterations_used = 0;
};
class HrnCircularRuleBasedLayoutSolver {
 public:
  static const unsigned kAlgorithmVersion = 1;
  HrnRuleLayoutResult solve(const HrnCircularLayoutRequest& request) const;
};
}  // namespace hrn
