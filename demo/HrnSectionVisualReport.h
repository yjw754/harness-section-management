#pragma once

#include <functional>
#include <string>
#include <vector>

#include "hrn/section/HrnPositionPointSolver.h"

namespace hrn {
struct HrnVisualNode {
  std::string id;
  std::string label;
  std::string parent_id;
  ZeHrnSide side = ZeHrnSide::Positive;
  size_t depth = 0;
  double input_radius = 0;
  double solved_radius = 0;
  double rotation = 0;
  double solved_rotation = 0;
  Point2D center2d{0, 0};
  Point3D location{0, 0, 0};
  bool has_geometry = false;
};
struct HrnVisualRelation {
  std::string positive_id;
  std::string negative_id;
  bool same_position = false;
};
struct HrnVisualReportModel {
  bool valid = false;
  bool solved = false;
  std::string error;
  Point3D position_center{0, 0, 0};
  Point2D x_axis{1, 0};
  Point2D y_axis{0, 1};
  std::vector<HrnVisualNode> nodes;
  std::vector<HrnVisualRelation> relations;
  std::vector<std::string> diagnostics;
  double side_root_radius[2] = {0, 0};
};
HrnVisualReportModel buildVisualReportModel(
    const HrnPositionPointInput& input,
    const std::vector<HrnSegmentSideRelation>& relations,
    const HrnCircularHierarchyResult& solve_result,
    const std::function<std::string(const HrnSegment&)>& labels);
std::string renderVisualReportHtml(const HrnVisualReportModel& model);
}  // namespace hrn
