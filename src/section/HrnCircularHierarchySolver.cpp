#include "hrn/section/HrnPositionPointSolver.h"

#include <cmath>
#include <functional>
#include <map>

#include "hrn/section/HrnPositionPoint.h"
#include "hrn/section/HrnRuleBasedLayoutSolver.h"
#include "hrn/section/HrnSectionGeometryService.h"

namespace hrn {
namespace {
const double kModelingTolerance = 1e-9;
const double kPi = 3.14159265358979323846;
Point2D rotate(Point2D point, double degrees) {
  const double angle = degrees * kPi / 180;
  const double c = std::cos(angle), s = std::sin(angle);
  return {point.x * c - point.y * s, point.x * s + point.y * c};
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
               HrnCircularHierarchyResult& output) {
  HrnCircularSolvedSide& solved = output.sides[side == ZeHrnSide::Positive ? 0 : 1];
  const size_t count = input.sectionCount(side);
  solved.root = input.sideRootSection(side).clone();
  if (!solved.root || !HrnPositionPoint::validateSectionData(*solved.root)) {
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
    if (!solved.sections.back() ||
        !HrnPositionPoint::validateSectionData(*solved.sections.back()) ||
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
    for (size_t member : members)
      request.members.push_back({member, solved.sections[member]->shape().dimensions().primary});
    const HrnRuleLayoutResult placed = layout_solver.solve(request);
    solved.candidate_count += placed.candidate_count;
    solved.iterations_used += placed.iterations_used;
    if (!placed.succeeded) {
      fail(output, target, HrnSectionFailureStage::LayoutSolve, placed.reason,
           request.settings.algorithm_version, placed.iterations_used);
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
      local[placement.id] = {placement.center.x - enclosure.circle.center.x,
                             placement.center.y - enclosure.circle.center.y};
      const auto containment = geometry.containsWithClearance(
          HrnCircle2D{{0, 0}, parent_radius},
          HrnCircle2D{local[placement.id], placement.radius}, request.spacing,
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
  for (size_t root : roots) if (!bottomUp(root)) return false;
  if (roots.size() > 1 &&
      !placeChildren(solved.root, roots, HrnSectionTarget::sideRoot(side))) return false;
  const double root_rotation = roots.size() > 1 ? solved.root->rotation() : 0;
  std::function<void(size_t, Point2D, double)> topDown =
      [&](size_t node, Point2D parent_center, double parent_rotation) {
        const Point2D offset = rotate(local[node], parent_rotation);
        const Point2D absolute{parent_center.x + offset.x, parent_center.y + offset.y};
        solved.centers2d[node] = absolute;
        solved.absolute_rotations[node] = normalizeDegrees(
            parent_rotation + solved.sections[node]->rotation());
        solved.locations[node] = transformPoint(input.positionTransform(), absolute);
        for (size_t child : children[node])
          topDown(child, absolute, solved.absolute_rotations[node]);
      };
  for (size_t root : roots) topDown(root, {0, 0}, root_rotation);
  return true;
}
}  // namespace

HrnCircularHierarchyResult HrnPositionPointSolver::solveCircular(
    const HrnPositionPointInput& input,
    const std::vector<HrnSegmentSideRelation>& relations) const {
  HrnCircularHierarchyResult result;
  (void)relations; // Explicit coupling is added by the Stage 2 transaction solver.
  if (!isRigidTransform(input.positionTransform(), kModelingTolerance)) {
    fail(result, HrnSectionTarget::positionPoint(), HrnSectionFailureStage::InputValidation,
         HrnSectionFailureReason::InvalidInput, 0, 0);
    return result;
  }
  if (!solveSide(input, ZeHrnSide::Positive, result) ||
      !solveSide(input, ZeHrnSide::Negative, result)) return result;
  result.status.committed = true;
  return result;
}
}  // namespace hrn
