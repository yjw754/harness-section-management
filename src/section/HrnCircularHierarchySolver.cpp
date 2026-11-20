#include "hrn/section/HrnPositionPointSolver.h"
#include "HrnSamePositionClasses.h"

#include <cmath>
#include <functional>
#include <iomanip>
#include <limits>
#include <locale>
#include <map>
#include <sstream>

#include "hrn/section/HrnPositionPoint.h"
#include "hrn/section/HrnRuleBasedLayoutSolver.h"
#include "hrn/section/HrnSectionGeometryService.h"

namespace hrn {
class HrnCircularPlacementSnapshot {
 public:
  std::string layout_key;
  HrnCircularRotationBaseline baseline;
  std::vector<double> radii[2];
  double root_radii[2]{};
};

bool enforceCoupledCircular(const HrnPositionPointInput& input,
                            const HrnSamePositionClasses& classes,
                            HrnCircularHierarchyResult& result, bool prefer_negative_seed,
                            const HrnCircularRotationBaseline* rotation = nullptr);
namespace {
const double kModelingTolerance = kHrnCircularModelingTolerance;
const double kPi = 3.14159265358979323846;
const ZeHrnSide kSides[] = {ZeHrnSide::Positive, ZeHrnSide::Negative};
Point2D rotate(Point2D point, double degrees) {
  const double angle = degrees * kPi / 180;
  const double c = std::cos(angle), s = std::sin(angle);
  return {point.x * c - point.y * s, point.x * s + point.y * c};
}
std::string layoutKey(const HrnPositionPointInput& input,
                      const std::vector<HrnSegmentSideRelation>& relations) {
  std::ostringstream key;
  key.imbue(std::locale::classic());
  key << std::hexfloat;
  for (const auto& row : input.positionTransform().m)
    for (double value : row) key << (value == 0 ? 0 : value) << ' ';
  const auto appendSection = [&](const HrnSectionData& section) {
    const auto dims = section.shape().dimensions();
    const auto& layout = section.layout();
    const auto& settings = layout.solveSettings();
    const double spacing = layout.spacingTolerance();
    key << static_cast<int>(dims.shape_type) << ' ' << dims.primary << ' ' << dims.secondary
        << ' ' << section.autoResizeEnabled() << ' ' << static_cast<int>(section.sizeUpdateMode())
        << ' ' << static_cast<int>(layout.type()) << ' ' << (spacing == 0 ? 0 : spacing)
        << ' ' << settings.algorithm_version << ' ' << settings.max_iterations
        << ' ' << settings.convergence_tolerance << ' ';
  };
  for (const auto side : kSides) {
    const size_t count = input.sectionCount(side);
    key << count << ' ';
    appendSection(input.sideRootSection(side));
    for (size_t index = 0; index < count; ++index) {
      size_t parent_index = count;
      const auto* parent = input.segmentAt(side, index).parent();
      if (parent) input.findSectionIndex(side, *parent, parent_index);
      key << parent_index << ' ';
      appendSection(input.sectionAt(side, index));
    }
  }
  key << relations.size() << ' ';
  for (const auto& relation : relations) {
    size_t positive = 0, negative = 0;
    input.findSectionIndex(kSides[0], *relation.positive, positive);
    input.findSectionIndex(kSides[1], *relation.negative, negative);
    key << positive << ' ' << negative << ' ' << relation.same_position << ' ';
  }
  return key.str();
}
void commitPlacement(HrnCircularHierarchyResult& result, const HrnPositionPointInput& input,
                     const std::string& layout_key, double frame_rotation) {
  result.status.committed = true;
  result.frame_rotation = normalizeDegrees(frame_rotation);
  auto snapshot = std::make_shared<HrnCircularPlacementSnapshot>();
  snapshot->layout_key = layout_key;
  snapshot->baseline.frame_rotation = result.frame_rotation;
  for (size_t side_index = 0; side_index < 2; ++side_index) {
    auto& side = result.sides[side_index];
    auto& prior = snapshot->baseline.sides[side_index];
    prior.centers = side.centers2d;
    prior.absolute_rotations = side.absolute_rotations;
    prior.root_rotation = input.sideRootSection(kSides[side_index]).rotation();
    prior.reference_root_rotation = side.reference_root_rotation;
    snapshot->root_radii[side_index] = side.root->shape().dimensions().primary;
    for (size_t index = 0; index < side.sections.size(); ++index) {
      prior.input_rotations.push_back(input.sectionAt(kSides[side_index], index).rotation());
      snapshot->radii[side_index].push_back(side.sections[index]->shape().dimensions().primary);
    }
    if (result.frame_rotation == 0) continue;
    side.reference_centers = side.centers2d;
    for (size_t index = 0; index < side.centers2d.size(); ++index) {
      const auto center = rotate(side.reference_centers[index], result.frame_rotation);
      side.centers2d[index] = center;
      side.absolute_rotations[index] =
          normalizeDegrees(side.absolute_rotations[index] + result.frame_rotation);
      side.locations[index] = transformPoint(input.positionTransform(), center);
    }
  }
  result.placement = std::move(snapshot);
}
double fallbackDelta(const HrnCircularRotationBaseline& baseline,
                     const HrnCircularHierarchyResult& attempted) {
  const double requested = normalizeDegrees(baseline.fallback_delta);
  if (requested != 0) return requested;
  const int preferred = baseline.prefer_negative ? 1 : 0;
  for (int side : {preferred, 1 - preferred}) {
    const auto& prior = baseline.sides[side].absolute_rotations;
    const auto& current = attempted.sides[side].absolute_rotations;
    for (size_t index = 0; index < current.size(); ++index) {
      const double delta = normalizeDegrees(current[index] - normalizeDegrees(prior[index]));
      if (delta != 0) return delta;
    }
  }
  return 0;
}
bool restoreSolvedSizes(HrnCircularHierarchyResult& result,
                        const HrnCircularPlacementSnapshot& snapshot) {
  const auto restore = [](std::unique_ptr<HrnSectionData>& section, double radius) {
    if (section->shape().dimensions().primary == radius) return true;
    if (!section->autoResizeEnabled()) return false;
    section = section->withCircularRadius(radius, kModelingTolerance);
    return section != nullptr;
  };
  for (size_t side = 0; side < 2; ++side) {
    auto& restored = result.sides[side];
    if (!restore(restored.root, snapshot.root_radii[side])) return false;
    for (size_t index = 0; index < restored.sections.size(); ++index)
      if (!restore(restored.sections[index], snapshot.radii[side][index])) return false;
  }
  return true;
}
HrnSectionTarget targetFor(ZeHrnSide side, const HrnCircularSolvedSide& data,
                           size_t index) {
  return HrnSectionTarget::segmentSection(side, *data.segments[index]);
}
void fail(HrnCircularHierarchyResult& result, HrnSectionTarget target,
          HrnSectionFailureStage stage, HrnSectionFailureReason reason,
          unsigned version, size_t iterations) {
  result.status.primary_reason = reason;
  result.status.diagnostics.push_back(
      makeFailureDiagnostic(target, stage, reason, version, iterations));
}
bool solveSide(const HrnPositionPointInput& input, ZeHrnSide side,
               const HrnSamePositionClasses& classes,
               HrnCircularHierarchyResult& output,
               const HrnCircularRotationBaseline::Side* rotation = nullptr,
               bool preserve_placement = false) {
  HrnCircularSolvedSide& solved = output.sides[side == ZeHrnSide::Positive ? 0 : 1];
  const size_t count = input.sectionCount(side);
  if (rotation && (rotation->centers.size() != count ||
                   rotation->absolute_rotations.size() != count ||
                   rotation->input_rotations.size() != count ||
                   !std::isfinite(rotation->root_rotation) ||
                   !std::isfinite(rotation->reference_root_rotation))) {
    fail(output, HrnSectionTarget::sideRoot(side), HrnSectionFailureStage::InputValidation,
         HrnSectionFailureReason::InvalidInput, 0, 0);
    return false;
  }
  solved.root = input.sideRootSection(side).clone();
  if (!solved.root || !HrnPositionPoint::validateSectionData(*solved.root) ||
      solved.root->layout().solveSettings().algorithm_version !=
          HrnCircularRuleBasedLayoutSolver::kAlgorithmVersion) {
    fail(output, HrnSectionTarget::sideRoot(side), HrnSectionFailureStage::InputValidation,
         HrnSectionFailureReason::InvalidInput, 0, 0);
    return false;
  }
  solved.segments.reserve(count);
  solved.sections.reserve(count);
  solved.centers2d.resize(count, {0, 0});
  solved.absolute_rotations.resize(count, 0);
  solved.locations.resize(count, {0, 0, 0});
  std::map<const HrnSegment*, size_t> index;
  for (size_t i = 0; i < count; ++i) {
    HrnSegment& segment = input.segmentAt(side, i);
    solved.segments.push_back(&segment);
    solved.sections.push_back(input.sectionAt(side, i).clone());
    if ((rotation && (!std::isfinite(rotation->centers[i].x) ||
                      !std::isfinite(rotation->centers[i].y) ||
                      !std::isfinite(rotation->absolute_rotations[i]) ||
                      !std::isfinite(rotation->input_rotations[i]))) ||
        !solved.sections.back() ||
        !HrnPositionPoint::validateSectionData(*solved.sections.back()) ||
        solved.sections.back()->layout().solveSettings().algorithm_version !=
            HrnCircularRuleBasedLayoutSolver::kAlgorithmVersion ||
        !index.insert(std::make_pair(&segment, i)).second) {
      fail(output, targetFor(side, solved, i), HrnSectionFailureStage::InputValidation,
           HrnSectionFailureReason::InvalidInput, 0, 0);
      return false;
    }
  }
  std::vector<std::vector<size_t>> children(count);
  std::vector<size_t> roots;
  for (size_t i = 0; i < count; ++i) {
    const auto parent = index.find(solved.segments[i]->parent());
    if (parent == index.end()) roots.push_back(i);
    else children[parent->second].push_back(i);
  }
  std::vector<Point2D> local(count, {0, 0});
  const HrnCircularRuleBasedLayoutSolver layout_solver;
  const HrnCircularGeometryService geometry;
  const auto placeChildren = [&](std::unique_ptr<HrnSectionData>& parent,
                                  const std::vector<size_t>& members,
                                  HrnSectionTarget target) -> bool {
    if (members.empty()) return true;
    HrnCircularLayoutRequest request;
    request.spacing = parent->layout().spacingTolerance();
    request.settings = parent->layout().solveSettings();
    // Same-center siblings share one envelope, but keep independent geometry.
    std::map<size_t, size_t> group_indices;
    std::vector<std::vector<size_t>> groups;
    for (size_t member : members) {
      const size_t group = classes.group(side, member);
      const auto inserted = group_indices.emplace(group, groups.size());
      if (inserted.second) {
        groups.push_back({});
        request.members.push_back({member, 0});
      }
      const size_t slot = inserted.first->second;
      groups[slot].push_back(member);
      const double radius = solved.sections[member]->shape().dimensions().primary;
      if (radius > request.members[slot].radius) request.members[slot] = {member, radius};
    }
    const HrnRuleLayoutResult placed = layout_solver.solve(request);
    solved.candidate_count += placed.candidate_count;
    solved.iterations_used += placed.iterations_used;
    if (!placed.succeeded) {
      fail(output, target, HrnSectionFailureStage::LayoutSolve, placed.reason,
           request.settings.algorithm_version, placed.iterations_used);
      if (placed.reason == HrnSectionFailureReason::IterationBudgetExceeded &&
          request.settings.max_iterations < std::numeric_limits<size_t>::max()) {
        HrnSectionAdjustment adjustment;
        adjustment.kind = HrnSectionAdjustmentKind::IncreaseIterationBudget;
        adjustment.target = target;
        adjustment.parameter = HrnSectionParameterKey::MaxIterations;
        adjustment.has_current_value = true;
        adjustment.current_value = HrnSectionAdjustmentValue::count(
            request.settings.max_iterations);
        adjustment.has_suggested_value = true;
        adjustment.suggested_value = HrnSectionAdjustmentValue::count(
            request.settings.max_iterations + 1);
        adjustment.certainty = HrnSectionAdjustmentCertainty::DeterministicCandidate;
        output.status.diagnostics.back().adjustments.push_back(adjustment);
      }
      return false;
    }
    std::vector<HrnCircle2D> disks;
    disks.reserve(placed.placements.size());
    for (const auto& placement : placed.placements)
      disks.push_back({placement.center, placement.radius});
    const auto enclosure = geometry.minimumEnclosingCircle(disks, kModelingTolerance);
    if (!enclosure.valid) {
      fail(output, target, HrnSectionFailureStage::GeometryValidation,
           HrnSectionFailureReason::GeometryValidationFailed,
           request.settings.algorithm_version, placed.iterations_used);
      return false;
    }
    const double required_radius = enclosure.circle.radius + request.spacing;
    const double current_radius = parent->shape().dimensions().primary;
    if (parent->autoResizeEnabled()) {
      parent = parent->withCircularRadius(required_radius, kModelingTolerance);
      if (!parent) {
        fail(output, target, HrnSectionFailureStage::ParentSizeUpdate,
             HrnSectionFailureReason::InvalidInput, request.settings.algorithm_version,
             placed.iterations_used);
        return false;
      }
    } else if (current_radius + kModelingTolerance < required_radius) {
      fail(output, target, HrnSectionFailureStage::GeometryValidation,
           HrnSectionFailureReason::FixedBoundaryInsufficient,
           request.settings.algorithm_version, placed.iterations_used);
      HrnSectionSolveDiagnostic& diagnostic = output.status.diagnostics.back();
      HrnSectionConflict conflict;
      conflict.kind = HrnSectionConflictKind::OutsideParent;
      conflict.lhs = target;
      conflict.has_measured_value = true;
      conflict.measured_value = current_radius;
      conflict.has_required_value = true;
      conflict.required_value = required_radius;
      const HrnCircularLayoutPlacement* worst = nullptr;
      double worst_extent = -1;
      for (const auto& placement : placed.placements) {
        const double x = placement.center.x - enclosure.circle.center.x;
        const double y = placement.center.y - enclosure.circle.center.y;
        const double extent = std::hypot(x, y) + placement.radius + request.spacing;
        if (extent > worst_extent) {
          worst_extent = extent;
          worst = &placement;
        }
      }
      if (worst) {
        const double x = worst->center.x - enclosure.circle.center.x;
        const double y = worst->center.y - enclosure.circle.center.y;
        const double length = std::hypot(x, y);
        const double ux = length == 0 ? 1 : x / length;
        const double uy = length == 0 ? 0 : y / length;
        conflict.has_rhs = true;
        conflict.rhs = targetFor(side, solved, worst->id);
        conflict.has_lhs_witness = true;
        conflict.lhs_witness = {ux * current_radius, uy * current_radius};
        conflict.has_rhs_witness = true;
        conflict.rhs_witness = {x + ux * worst->radius, y + uy * worst->radius};
      }
      diagnostic.conflicts.push_back(conflict);
      HrnSectionAdjustment adjustment;
      adjustment.kind = HrnSectionAdjustmentKind::IncreaseParentDimension;
      adjustment.target = target;
      adjustment.parameter = HrnSectionParameterKey::Radius;
      adjustment.has_current_value = true;
      adjustment.current_value = HrnSectionAdjustmentValue::number(current_radius);
      adjustment.has_suggested_value = true;
      adjustment.suggested_value = HrnSectionAdjustmentValue::number(required_radius);
      adjustment.certainty = HrnSectionAdjustmentCertainty::SufficientForCurrentPlacement;
      diagnostic.adjustments.push_back(adjustment);
      return false;
    }
    const double parent_radius = parent->shape().dimensions().primary;
    for (const auto& placement : placed.placements) {
      const Point2D center{placement.center.x - enclosure.circle.center.x,
                           placement.center.y - enclosure.circle.center.y};
      for (size_t member : groups[group_indices.at(classes.group(side, placement.id))])
        local[member] = center;
      const auto containment = geometry.containsWithClearance(
          HrnCircle2D{{0, 0}, parent_radius},
          HrnCircle2D{center, placement.radius}, request.spacing,
          kModelingTolerance);
      if (!containment.valid || !containment.contained) {
        fail(output, target, HrnSectionFailureStage::GeometryValidation,
             HrnSectionFailureReason::GeometryValidationFailed,
             request.settings.algorithm_version, placed.iterations_used);
        return false;
      }
    }
    return true;
  };
  std::function<bool(size_t)> bottomUp = [&](size_t node) -> bool {
    for (size_t child : children[node]) if (!bottomUp(child)) return false;
    return placeChildren(solved.sections[node], children[node], targetFor(side, solved, node));
  };
  if (!rotation) {
    for (size_t root : roots) if (!bottomUp(root)) return false;
    if (roots.size() > 1 &&
        !placeChildren(solved.root, roots, HrnSectionTarget::sideRoot(side))) return false;
  }
  const double root_rotation = roots.size() > 1 ? solved.root->rotation() : 0;
  const double root_delta = rotation && roots.size() > 1 ?
      normalizeDegrees(root_rotation - normalizeDegrees(rotation->root_rotation)) : 0;
  solved.reference_root_rotation = rotation ? normalizeDegrees(
      normalizeDegrees(rotation->reference_root_rotation) + (preserve_placement ? 0 : root_delta)) :
      root_rotation;
  std::function<void(size_t, size_t, Point2D, double, double)> topDown =
      [&](size_t node, size_t parent, Point2D parent_center, double parent_rotation,
          double parent_delta) {
        Point2D absolute;
        const double local_rotation = solved.sections[node]->rotation();
        double node_delta = 0;
        double node_rotation = normalizeDegrees(parent_rotation + local_rotation);
        if (rotation) {
          const Point2D previous = rotation->centers[node];
          const Point2D pivot = parent == count ? Point2D{0, 0} : rotation->centers[parent];
          node_delta = preserve_placement ? 0 : normalizeDegrees(
              parent_delta + (local_rotation - normalizeDegrees(rotation->input_rotations[node])));
          node_rotation = normalizeDegrees(
              normalizeDegrees(rotation->absolute_rotations[node]) + node_delta);
          if (preserve_placement) {
            absolute = previous;
          } else if (parent_delta == 0) {
            absolute = {previous.x + (parent_center.x - pivot.x),
                        previous.y + (parent_center.y - pivot.y)};
          } else {
            const Point2D offset = rotate({previous.x - pivot.x, previous.y - pivot.y}, parent_delta);
            absolute = {parent_center.x + offset.x, parent_center.y + offset.y};
          }
        } else {
          const Point2D offset = rotate(local[node], parent_rotation);
          absolute = {parent_center.x + offset.x, parent_center.y + offset.y};
        }
        solved.centers2d[node] = absolute;
        solved.absolute_rotations[node] = node_rotation;
        solved.locations[node] = transformPoint(input.positionTransform(), absolute);
        for (size_t child : children[node])
          topDown(child, node, absolute, node_rotation, node_delta);
      };
  for (size_t root : roots) topDown(root, count, {0, 0}, root_rotation, root_delta);
  return true;
}
}  // namespace

HrnCircularHierarchyResult HrnPositionPointSolver::solveCircular(
    const HrnPositionPointInput& input,
    const std::vector<HrnSegmentSideRelation>& relations,
    const HrnCircularRotationBaseline* rotation) const {
  HrnCircularHierarchyResult result;
  if (!isRigidTransform(input.positionTransform(), kModelingTolerance) ||
      (rotation && (!std::isfinite(rotation->fallback_delta) ||
                    !std::isfinite(rotation->frame_rotation)))) {
    fail(result, HrnSectionTarget::positionPoint(), HrnSectionFailureStage::InputValidation,
         HrnSectionFailureReason::InvalidInput, 0, 0);
    return result;
  }
  HrnSamePositionClasses classes;
  if (!classes.build(input, relations)) {
    fail(result, HrnSectionTarget::positionPoint(), HrnSectionFailureStage::InputValidation,
         HrnSectionFailureReason::InvalidInput, 0, 0);
    return result;
  }
  const auto key = layoutKey(input, relations);
  const auto* snapshot = rotation ? rotation->placement.get() : nullptr;
  HrnCircularRotationBaseline resolved;
  if (snapshot) {
    if (key != snapshot->layout_key) {
      fail(result, HrnSectionTarget::positionPoint(), HrnSectionFailureStage::InputValidation,
           HrnSectionFailureReason::InvalidInput, 0, 0);
      return result;
    }
    resolved = snapshot->baseline;
    resolved.fallback_delta = rotation->fallback_delta;
    resolved.prefer_negative = rotation->prefer_negative;
    rotation = &resolved;
  }
  if (!solveSide(input, ZeHrnSide::Positive, classes, result,
                 rotation ? &rotation->sides[0] : nullptr) ||
      !solveSide(input, ZeHrnSide::Negative, classes, result,
                 rotation ? &rotation->sides[1] : nullptr)) return result;
  if (!enforceCoupledCircular(input, classes, result, rotation && rotation->prefer_negative,
                              rotation)) {
    if (rotation) {
      const double delta = fallbackDelta(*rotation, result);
      HrnCircularHierarchyResult retry;
      if ((snapshot || delta != 0) &&
          solveSide(input, ZeHrnSide::Positive, classes, retry, &rotation->sides[0], true) &&
          solveSide(input, ZeHrnSide::Negative, classes, retry, &rotation->sides[1], true) &&
          (snapshot ? restoreSolvedSizes(retry, *snapshot) :
           enforceCoupledCircular(input, classes, retry, rotation->prefer_negative, rotation))) {
        retry.used_global_rotation = delta != 0;
        commitPlacement(retry, input, key, normalizeDegrees(rotation->frame_rotation) + delta);
        return retry;
      }
      return result;
    }
    if (result.status.primary_reason != HrnSectionFailureReason::NoSolutionFound) return result;
    // A seeded collision or boundary violation is not a proof of infeasibility. Retry once
    // with the opposite independent seeds, without using mutated geometry.
    HrnCircularHierarchyResult retry;
    if (!solveSide(input, ZeHrnSide::Positive, classes, retry) ||
        !solveSide(input, ZeHrnSide::Negative, classes, retry)) return result;
    if (!enforceCoupledCircular(input, classes, retry, true) &&
        retry.status.primary_reason != HrnSectionFailureReason::SamePositionConstraintConflict)
      return result;
    result = std::move(retry);
    if (result.status.primary_reason != HrnSectionFailureReason::None) return result;
  }
  commitPlacement(result, input, key, rotation ? rotation->frame_rotation : 0);
  return result;
}
}  // namespace hrn
