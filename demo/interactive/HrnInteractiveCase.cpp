#include "HrnInteractiveCase.h"

#include <cmath>
#include <map>
#include <memory>
#include <set>
#include <utility>

#include "HrnSectionVisualReport.h"

namespace hrn {
class HrnInteractivePlacementSnapshot {
 public:
  std::shared_ptr<const HrnCircularPlacementSnapshot> circular;
  std::vector<std::string> section_ids;
};

namespace {
constexpr const char* kSideNames[] = {"positive", "negative"};
constexpr ZeHrnSide kSides[] = {ZeHrnSide::Positive, ZeHrnSide::Negative};
using SectionIndices = std::map<std::string, size_t>;

std::string memberPath(size_t index) { return "sections[" + std::to_string(index) + "]"; }

void validateOptions(const HrnInteractiveSectionOptions& options, const std::string& path,
                     double tolerance, std::vector<HrnInteractiveFieldError>& errors) {
  if (options.shape.kind != "circular")
    errors.push_back({path + ".shape.kind", "当前仅支持圆形截面"});
  else if (!std::isfinite(options.shape.radius) || options.shape.radius <= tolerance)
    errors.push_back({path + ".shape.radius", "半径必须为有限数字且大于建模公差"});
  if (!std::isfinite(options.rotation))
    errors.push_back({path + ".rotation", "旋转角必须为有限数字"});
  if (!std::isfinite(options.spacing) || options.spacing < 0)
    errors.push_back({path + ".spacing", "间距公差必须为有限非负数"});
}

std::unique_ptr<HrnSectionData> makeSection(const HrnInteractiveSectionOptions& input,
                                            double tolerance) {
  if (input.shape.kind != "circular") return nullptr;
  auto shape = HrnCircularSectionShape::create(input.shape.radius, tolerance);
  auto layout = std::make_unique<HrnRuleBasedLayout>();
  if (!layout->setSpacingTolerance(input.spacing)) return nullptr;
  HrnSectionOptions options;
  options.rotation = input.rotation;
  options.auto_resize_enabled = input.auto_resize;
  return HrnSectionData::create(std::move(shape), std::move(layout), options);
}

HrnInteractiveCaseResult invalid(const std::string& path, const std::string& message) {
  HrnInteractiveCaseResult result;
  result.errors.push_back({path, message});
  return result;
}
}  // namespace

std::vector<HrnInteractiveFieldError> validateInteractiveCase(
    const HrnInteractiveCaseDefinition& definition) {
  std::vector<HrnInteractiveFieldError> errors;
  const double tolerance = definition.modeling_tolerance;
  if (definition.schema_version != 1) errors.push_back({"schemaVersion", "仅支持版本 1"});
  if (tolerance != kHrnCircularModelingTolerance)
    errors.push_back({"modelingTolerance", "当前圆形求解器的建模公差仅支持 1e-9"});
  if (!isRigidTransform(definition.position_transform, 1e-9))
    errors.push_back({"positionTransform", "必须为有限的刚体变换矩阵"});
  for (size_t i = 0; i < definition.roots.size(); ++i)
    validateOptions(definition.roots[i], std::string("roots.") + kSideNames[i], tolerance, errors);
  const size_t count = definition.sections.size();
  SectionIndices indices;
  for (size_t i = 0; i < count; ++i) {
    const auto& section = definition.sections[i];
    const std::string path = memberPath(i);
    if (section.id.empty() || !indices.emplace(section.id, i).second)
      errors.push_back({path + ".id", "Section ID 必须非空且唯一"});
    if (section.side != kSideNames[0] && section.side != kSideNames[1])
      errors.push_back({path + ".side", "无效的 Section 侧别"});
    validateOptions(section.options, path, tolerance, errors);
  }

  std::vector<size_t> parents(count, count);
  for (size_t i = 0; i < count; ++i) {
    const auto& section = definition.sections[i];
    if (section.parent_id.empty()) continue;
    const auto parent = indices.find(section.parent_id);
    if (parent == indices.end() || parent->second == i ||
        definition.sections[parent->second].side != section.side) {
      errors.push_back({memberPath(i) + ".parentId", "父 Section 必须存在、同侧且不是自身"});
    } else {
      parents[i] = parent->second;
    }
  }
  enum Visit { Unseen, Visiting, Done };
  std::vector<Visit> visits(count, Unseen);
  for (size_t start = 0; start < count; ++start) {
    size_t current = start;
    while (current != count && visits[current] == Unseen) {
      visits[current] = Visiting;
      current = parents[current];
    }
    if (current != count && visits[current] == Visiting)
      errors.push_back({memberPath(current) + ".parentId", "嵌套关系不能成环"});
    for (current = start; current != count && visits[current] == Visiting; current = parents[current])
      visits[current] = Done;
  }

  std::set<std::pair<std::string, std::string>> pairs;
  for (size_t i = 0; i < definition.relations.size(); ++i) {
    const auto& relation = definition.relations[i];
    const std::string path = "relations[" + std::to_string(i) + "]";
    const auto p = indices.find(relation.positive_id);
    const auto n = indices.find(relation.negative_id);
    if (p == indices.end() || definition.sections[p->second].side != kSideNames[0])
      errors.push_back({path + ".positiveId", "端点必须是已存在的正侧 Section"});
    if (n == indices.end() || definition.sections[n->second].side != kSideNames[1])
      errors.push_back({path + ".negativeId", "端点必须是已存在的负侧 Section"});
    if (!pairs.emplace(relation.positive_id, relation.negative_id).second)
      errors.push_back({path, "映射端点对不可重复"});
  }
  return errors;
}

HrnInteractiveCaseResult solveInteractiveCase(
    const HrnInteractiveCaseDefinition& definition,
    const HrnInteractiveRotationBaseline* rotation) {
  HrnInteractiveCaseResult result;
  result.errors = validateInteractiveCase(definition);
  if (!result.errors.empty()) return result;

  const double tolerance = definition.modeling_tolerance;
  auto positive = makeSection(definition.roots[0], tolerance);
  auto negative = makeSection(definition.roots[1], tolerance);
  if (!positive || !negative) return invalid("roots", "无法创建侧 Root");

  std::vector<std::unique_ptr<HrnSegment>> segments;
  std::vector<std::string> section_ids;
  SectionIndices indices;
  for (const auto& section : definition.sections) {
    indices.emplace(section.id, segments.size());
    segments.push_back(std::make_unique<HrnSegment>());
    section_ids.push_back(section.id);
  }
  auto input = HrnPositionPointInput::create(definition.position_transform, std::move(positive),
                                              std::move(negative));
  if (!input) return invalid("positionTransform", "无法创建位置点输入");
  std::map<const HrnSegment*, std::string> labels;
  std::map<std::string, size_t> positional_indices;
  HrnCircularRotationBaseline baseline;
  if (rotation) {
    if (rotation->placement && rotation->placement->section_ids != section_ids)
      return invalid("rotationBaseline.placement", "布局快照的成员 ID 或顺序与当前案例不一致");
    if (!rotation->placement && rotation->nodes.size() != definition.sections.size())
      return invalid("rotationBaseline.nodes", "旋转基准的成员与当前案例不一致");
    if (rotation->placement) baseline.placement = rotation->placement->circular;
    baseline.prefer_negative = rotation->prefer_negative;
    baseline.fallback_delta = rotation->fallback_delta;
    baseline.frame_rotation = normalizeDegrees(rotation->frame_rotation);
    for (size_t side = 0; side < 2; ++side) {
      baseline.sides[side].root_rotation = rotation->root_rotations[side];
      baseline.sides[side].reference_root_rotation = rotation->reference_root_rotations.size() == 2 ?
          rotation->reference_root_rotations[side] : rotation->root_rotations[side];
    }
  }
  for (size_t i = 0; i < definition.sections.size(); ++i) {
    const auto& section = definition.sections[i];
    if (!section.parent_id.empty() && !segments[i]->attachTo(*segments[indices.at(section.parent_id)]))
      return invalid(memberPath(i) + ".parentId", "无法建立嵌套关系");
    const size_t side = section.side == kSideNames[0] ? 0 : 1;
    if (rotation && !rotation->placement) {
      const auto previous = rotation->nodes.find(section.id);
      if (previous == rotation->nodes.end())
        return invalid("rotationBaseline.nodes", "旋转基准缺少截面 " + section.id);
      baseline.sides[side].centers.push_back(previous->second.center);
      baseline.sides[side].absolute_rotations.push_back(
          normalizeDegrees(previous->second.absolute_rotation) - baseline.frame_rotation);
      baseline.sides[side].input_rotations.push_back(previous->second.input_rotation);
    }
    const std::string positional = (side == 0 ? "p" : "n") +
                                   std::to_string(input->sectionCount(kSides[side]));
    if (!input->append(kSides[side], *segments[i], makeSection(section.options, tolerance)))
      return invalid(memberPath(i), "无法创建 Section");
    positional_indices.emplace(positional, i);
    labels.emplace(segments[i].get(), section.label);
  }
  std::vector<HrnSegmentSideRelation> relations;
  for (const auto& relation : definition.relations) {
    relations.push_back({segments[indices.at(relation.positive_id)].get(),
                         segments[indices.at(relation.negative_id)].get(), relation.same_position});
  }

  const auto solve_result = HrnPositionPointSolver().solveCircular(
      *input, relations, rotation ? &baseline : nullptr);
  const auto visual = buildVisualReportModel(*input, relations, solve_result,
      [&](const HrnSegment& segment) { return labels.at(&segment); });
  if (!visual.valid) return invalid("$", visual.error);
  result.valid = true;
  result.solved = visual.solved;
  if (solve_result.placement)
    result.placement = std::make_shared<HrnInteractivePlacementSnapshot>(
        HrnInteractivePlacementSnapshot{solve_result.placement, std::move(section_ids)});
  result.used_global_rotation = solve_result.used_global_rotation;
  result.frame_rotation = solve_result.frame_rotation;
  result.diagnostics = visual.diagnostics;
  if (result.used_global_rotation)
    result.diagnostics.push_back("局部旋转无法满足约束，已整体旋转正负两侧");
  for (size_t side = 0; side < result.roots.size(); ++side) {
    auto& root = result.roots[side];
    root.input_shape = definition.roots[side].shape;
    if (visual.solved) root.solved_shape = {"circular", visual.side_root_radius[side]};
    root.reference_rotation = solve_result.side(kSides[side]).reference_root_rotation;
    root.candidate_count = solve_result.side(kSides[side]).candidate_count;
    root.iterations_used = solve_result.side(kSides[side]).iterations_used;
  }
  auto original = [&](const std::string& positional) -> const HrnInteractiveSectionDefinition& {
    return definition.sections[positional_indices.at(positional)];
  };
  size_t indices_by_side[2]{};
  for (const auto& node : visual.nodes) {
    HrnInteractiveNodeResult mapped;
    const auto& section = original(node.id);
    mapped.id = section.id;
    mapped.label = node.label;
    mapped.parent_id = node.parent_id.empty() ? "" : original(node.parent_id).id;
    const size_t side = node.side == kSides[0] ? 0 : 1;
    const size_t index = indices_by_side[side]++;
    mapped.side = kSideNames[side];
    mapped.depth = node.depth;
    mapped.input_shape = section.options.shape;
    mapped.input_rotation = section.options.rotation;
    mapped.has_geometry = node.has_geometry;
    if (node.has_geometry) {
      mapped.solved_shape = {"circular", node.solved_radius};
      mapped.solved_rotation = node.solved_rotation;
      mapped.center = node.center2d;
      const auto& reference = solve_result.sides[side].reference_centers;
      mapped.reference_center = reference.empty() ? mapped.center : reference[index];
    }
    result.nodes.push_back(std::move(mapped));
  }
  for (const auto& relation : visual.relations) {
    result.relations.push_back({original(relation.positive_id).id,
        original(relation.negative_id).id, relation.same_position});
  }
  return result;
}
}  // namespace hrn
