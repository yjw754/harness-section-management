#pragma once

#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "hrn/section/HrnGeometryTypes.h"

namespace hrn {
class HrnInteractivePlacementSnapshot;
struct HrnInteractiveShape {
  std::string kind;
  double radius = 0;
};
struct HrnInteractiveSectionOptions {
  HrnInteractiveShape shape;
  double rotation = 0;
  double spacing = 0;
  bool auto_resize = false;
};
struct HrnInteractiveSectionDefinition {
  std::string id;
  std::string label;
  std::string side;
  std::string parent_id;  // Empty is the native representation of JSON null.
  HrnInteractiveSectionOptions options;
};
struct HrnInteractiveRelation {
  std::string positive_id;
  std::string negative_id;
  bool same_position = true;
};
struct HrnInteractiveCaseDefinition {
  unsigned schema_version = 1;
  double modeling_tolerance = 1e-9;
  Matrix4D position_transform;
  std::array<HrnInteractiveSectionOptions, 2> roots;  // Positive, negative.
  std::vector<HrnInteractiveSectionDefinition> sections;
  std::vector<HrnInteractiveRelation> relations;
};
struct HrnInteractiveFieldError {
  std::string path;
  std::string message;
};
struct HrnInteractiveRootResult {
  HrnInteractiveShape input_shape;
  HrnInteractiveShape solved_shape;
  double reference_rotation = 0;
  size_t candidate_count = 0;
  size_t iterations_used = 0;
};
struct HrnInteractiveNodeResult {
  std::string id;
  std::string label;
  std::string side;
  std::string parent_id;
  size_t depth = 0;
  HrnInteractiveShape input_shape;
  double input_rotation = 0;
  bool has_geometry = false;
  HrnInteractiveShape solved_shape;
  double solved_rotation = 0;
  Point2D center{0, 0};
  Point2D reference_center{0, 0};
};
struct HrnInteractiveCaseResult {
  bool valid = false;
  bool solved = false;
  bool used_global_rotation = false;
  double frame_rotation = 0;
  std::shared_ptr<const HrnInteractivePlacementSnapshot> placement;
  std::vector<HrnInteractiveFieldError> errors;
  std::vector<std::string> diagnostics;
  std::array<HrnInteractiveRootResult, 2> roots;
  std::vector<HrnInteractiveNodeResult> nodes;
  std::vector<HrnInteractiveRelation> relations;
};
struct HrnInteractiveRotationNode {
  Point2D center{0, 0};
  double absolute_rotation = 0;
  double input_rotation = 0;
};
struct HrnInteractiveRotationBaseline {
  std::array<double, 2> root_rotations{};
  std::vector<double> reference_root_rotations;
  std::map<std::string, HrnInteractiveRotationNode> nodes;
  bool prefer_negative = false;
  double fallback_delta = 0;
  double frame_rotation = 0;
  std::shared_ptr<const HrnInteractivePlacementSnapshot> placement;
};

std::vector<HrnInteractiveFieldError> validateInteractiveCase(
    const HrnInteractiveCaseDefinition& definition);
HrnInteractiveCaseResult solveInteractiveCase(
    const HrnInteractiveCaseDefinition& definition,
    const HrnInteractiveRotationBaseline* rotation = nullptr);
}  // namespace hrn
