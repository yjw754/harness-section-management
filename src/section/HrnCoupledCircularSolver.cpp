#include "hrn/section/HrnPositionPointSolver.h"
#include "HrnSamePositionClasses.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <vector>

#include "hrn/section/HrnSectionGeometryService.h"
#include "hrn/section/HrnRuleBasedLayoutSolver.h"

namespace hrn {
namespace {
const double kTolerance = kHrnCircularModelingTolerance;
struct NodeRef { int side; size_t index; };
struct Tree {
  std::vector<int> parent;
  std::vector<std::vector<size_t>> children;
  std::vector<size_t> roots;
  std::vector<bool> pinned_subtree;
};
Point2D centerOf(const HrnCircularHierarchyResult& result, NodeRef ref) {
  return result.sides[ref.side].centers2d[ref.index];
}
double radiusOf(const HrnCircularHierarchyResult& result, NodeRef ref) {
  return result.sides[ref.side].sections[ref.index]->shape().dimensions().primary;
}
HrnSectionTarget targetOf(const HrnCircularHierarchyResult& result, NodeRef ref) {
  return HrnSectionTarget::segmentSection(ref.side == 0 ? ZeHrnSide::Positive :
      ZeHrnSide::Negative, *result.sides[ref.side].segments[ref.index]);
}
void fail(HrnCircularHierarchyResult& result, HrnSectionTarget target,
          HrnSectionFailureReason reason, HrnSectionFailureStage stage) {
  result.status.committed = false;
  result.status.primary_reason = reason;
  result.status.diagnostics.push_back(makeFailureDiagnostic(target, stage, reason, 1, 0));
}
void addBoundaryEvidence(HrnCircularHierarchyResult& result,
                         HrnSectionTarget parent_target, HrnSectionTarget child_target,
                         Point2D parent_center, Point2D child_center,
                         double child_radius, double current_radius, double required_radius) {
  HrnSectionConflict conflict;
  conflict.kind = HrnSectionConflictKind::OutsideParent;
  conflict.lhs = parent_target;
  conflict.has_rhs = true;
  conflict.rhs = child_target;
  conflict.has_measured_value = true;
  conflict.measured_value = current_radius;
  conflict.has_required_value = true;
  conflict.required_value = required_radius;
  const double dx = child_center.x - parent_center.x;
  const double dy = child_center.y - parent_center.y;
  const double length = std::hypot(dx, dy);
  const double ux = length == 0 ? 1 : dx / length;
  const double uy = length == 0 ? 0 : dy / length;
  conflict.has_lhs_witness = true;
  conflict.lhs_witness = {parent_center.x + ux * current_radius,
                          parent_center.y + uy * current_radius};
  conflict.has_rhs_witness = true;
  conflict.rhs_witness = {child_center.x + ux * child_radius,
                          child_center.y + uy * child_radius};
  result.status.diagnostics.back().conflicts.push_back(conflict);
  HrnSectionAdjustment adjustment;
  adjustment.kind = HrnSectionAdjustmentKind::IncreaseParentDimension;
  adjustment.target = parent_target;
  adjustment.parameter = HrnSectionParameterKey::Radius;
  adjustment.has_current_value = true;
  adjustment.current_value = HrnSectionAdjustmentValue::number(current_radius);
  adjustment.has_suggested_value = true;
  adjustment.suggested_value = HrnSectionAdjustmentValue::number(required_radius);
  adjustment.certainty = result.status.primary_reason ==
      HrnSectionFailureReason::NoSolutionFound ?
      HrnSectionAdjustmentCertainty::HeuristicSuggestion :
      HrnSectionAdjustmentCertainty::SufficientForCurrentPlacement;
  result.status.diagnostics.back().adjustments.push_back(adjustment);
}
Tree makeTree(const HrnCircularSolvedSide& side) {
  Tree tree;
  const size_t count = side.segments.size();
  tree.parent.assign(count, -1);
  tree.children.resize(count);
  tree.pinned_subtree.assign(count, false);
  std::map<const HrnSegment*, size_t> index;
  for (size_t i = 0; i < count; ++i) index[side.segments[i]] = i;
  for (size_t i = 0; i < count; ++i) {
    const auto found = index.find(side.segments[i]->parent());
    if (found == index.end()) tree.roots.push_back(i);
    else {
      tree.parent[i] = static_cast<int>(found->second);
      tree.children[found->second].push_back(i);
    }
  }
  return tree;
}
void shift(HrnCircularHierarchyResult& result, const Tree& tree, NodeRef ref,
           Point2D delta) {
  HrnCircularSolvedSide& side = result.sides[ref.side];
  side.centers2d[ref.index].x += delta.x;
  side.centers2d[ref.index].y += delta.y;
  for (size_t child : tree.children[ref.index]) shift(result, tree, {ref.side, child}, delta);
}
bool same(Point2D a, Point2D b) {
  return std::hypot(a.x - b.x, a.y - b.y) <= kTolerance;
}
Point2D rotate(Point2D point, double degrees) {
  const double angle = degrees * 3.14159265358979323846 / 180;
  const double c = std::cos(angle), s = std::sin(angle);
  return {c * point.x - s * point.y, s * point.x + c * point.y};
}

// Rebuild a whole free family around the mapped obstacles. Keep the old
// geometry until every free envelope has a valid candidate.
bool repackFreeSiblings(HrnCircularHierarchyResult& result, int side_index,
                        const Tree& tree, const std::vector<size_t>& children,
                        const HrnSamePositionClasses& classes) {
  auto& side = result.sides[side_index];
  const auto side_name = side_index == 0 ? ZeHrnSide::Positive : ZeHrnSide::Negative;
  const int parent = tree.parent[children.front()];
  const auto& container = parent == -1 ? *side.root : *side.sections[parent];
  const Point2D parent_center = parent == -1 ? Point2D{0, 0} : side.centers2d[parent];
  const double angle = parent == -1 ? side.reference_root_rotation : side.absolute_rotations[parent];
  HrnCircularLayoutRequest request;
  request.spacing = container.layout().spacingTolerance();
  request.settings = container.layout().solveSettings();
  if (!container.autoResizeEnabled())
    request.parent_radius_limit = container.shape().dimensions().primary;
  std::map<size_t, std::vector<size_t>> members;
  std::vector<size_t> group_order;
  for (size_t child : children) {
    const size_t group = classes.group(side_name, child);
    if (members[group].empty()) group_order.push_back(group);
    members[group].push_back(child);
  }
  std::vector<HrnCircle2D> old_circles;
  double old_envelope = 0, old_compactness = 0;
  for (size_t group : group_order) {
    size_t representative = members[group].front();
    for (size_t child : members[group])
      if (radiusOf(result, {side_index, child}) > radiusOf(result, {side_index, representative}))
        representative = child;
    const auto center = side.centers2d[representative];
    const auto local = rotate({center.x - parent_center.x, center.y - parent_center.y}, -angle);
    const double radius = radiusOf(result, {side_index, representative});
    old_circles.push_back({local, radius});
    old_envelope = std::max(old_envelope, std::hypot(local.x, local.y) + radius + request.spacing);
    old_compactness += local.x * local.x + local.y * local.y;
    if (tree.pinned_subtree[representative])
      request.fixed_placements.push_back({representative, radius, local});
    else request.members.push_back({representative, radius});
  }
  if (request.fixed_placements.empty() || request.members.empty()) return false;
  const auto placed = HrnCircularRuleBasedLayoutSolver().solve(request);
  side.candidate_count += placed.candidate_count;
  side.iterations_used += placed.iterations_used;
  if (!placed.succeeded) return false;
  const HrnCircularGeometryService geometry;
  bool old_valid = old_envelope <= request.parent_radius_limit + kTolerance;
  for (size_t i = 0; old_valid && i < old_circles.size(); ++i)
    for (size_t j = i + 1; j < old_circles.size(); ++j) {
      const auto distance = geometry.signedDistance(old_circles[i], old_circles[j], kTolerance);
      if (!distance.valid || distance.signed_distance + kTolerance < request.spacing) old_valid = false;
    }
  double compactness = 0;
  for (const auto& placement : placed.placements)
    compactness += placement.center.x * placement.center.x + placement.center.y * placement.center.y;
  if (old_valid && (placed.required_parent_radius > old_envelope + kTolerance ||
      (placed.required_parent_radius >= old_envelope - kTolerance &&
       compactness >= old_compactness - kTolerance))) return false;
  for (const auto& placement : placed.placements) {
    if (tree.pinned_subtree[placement.id]) continue;
    const auto offset = rotate(placement.center, angle);
    const Point2D target{parent_center.x + offset.x, parent_center.y + offset.y};
    for (size_t child : members.at(classes.group(side_name, placement.id))) {
      const auto current = side.centers2d[child];
      shift(result, tree, {side_index, child}, {target.x - current.x, target.y - current.y});
    }
  }
  return true;
}

// Propose a parent-only move. The direct children keep their absolute
// centers; their enclosing circle is independent of the old parent center.
bool recenteredParent(const HrnCircularSolvedSide& side, const Tree& tree,
                      size_t node, const HrnCircularGeometryService& geometry,
                      HrnCircle2D& proposal) {
  const auto& section = *side.sections[node];
  const double spacing = section.layout().spacingTolerance();
  std::vector<HrnCircle2D> children;
  for (size_t child : tree.children[node])
    children.push_back({side.centers2d[child],
                        side.sections[child]->shape().dimensions().primary});
  const auto enclosure = geometry.minimumEnclosingCircle(children, kTolerance);
  if (!enclosure.valid) return false;
  const double required = enclosure.circle.radius + spacing;
  if (!std::isfinite(required)) return false;
  proposal = {enclosure.circle.center, section.autoResizeEnabled() ? required :
      section.shape().dimensions().primary};
  if (proposal.radius + kTolerance < required) return false;
  const int parent = tree.parent[node];
  const auto& container = parent == -1 ? *side.root : *side.sections[parent];
  const auto center = parent == -1 ? Point2D{0, 0} : side.centers2d[parent];
  if (container.autoResizeEnabled()) return true;
  return geometry.containsWithClearance({center, container.shape().dimensions().primary},
      proposal, container.layout().spacingTolerance(), kTolerance).contained;
}
}  // namespace

bool enforceCoupledCircular(const HrnPositionPointInput& input,
                            const HrnSamePositionClasses& classes,
                            HrnCircularHierarchyResult& result, bool prefer_negative_seed,
                            const HrnCircularRotationBaseline* rotation) {
  const size_t positive_count = result.sides[0].segments.size();
  const size_t negative_count = result.sides[1].segments.size();
  const size_t total = positive_count + negative_count;
  if (classes.edgeCount() == 0 && !rotation) return true;
  Tree trees[2] = {makeTree(result.sides[0]), makeTree(result.sides[1])};
  const std::vector<Point2D> seed_centers[2] = {result.sides[0].centers2d, result.sides[1].centers2d};
  // A singleton without descendants can follow the only explicit link to a
  // multi-root side without changing either side's containment constraints.
  int following_side = -1;
  if (classes.edgeCount() == 1) {
    if (positive_count == 1 && trees[1].roots.size() > 1) following_side = 0;
    if (negative_count == 1 && trees[0].roots.size() > 1) following_side = 1;
  }
  const auto anchoredAtOrigin = [&](NodeRef ref) {
    return ref.side != following_side && trees[ref.side].roots.size() == 1 &&
        trees[ref.side].parent[ref.index] == -1;
  };
  std::map<size_t, std::vector<NodeRef>> groups;
  for (size_t i = 0; i < total; ++i) {
    const size_t leader = classes.group(i);
    groups[leader].push_back(i < positive_count ? NodeRef{0, i} :
        NodeRef{1, i - positive_count});
  }
  for (auto it = groups.begin(); it != groups.end();) {
    if (it->second.size() < 2) it = groups.erase(it);
    else ++it;
  }
  // The eligible singleton uses its peer's rule-based center. Other lone
  // roots stay anchored and force their component to the Position Point origin.
  std::map<size_t, Point2D> shared;
  std::vector<bool> directly_mapped(total, false);
  const auto globalIndex = [positive_count](NodeRef ref) {
    return ref.side == 0 ? ref.index : positive_count + ref.index;
  };
  for (const auto& group : groups) {
    Point2D target = centerOf(result, prefer_negative_seed ?
        group.second.back() : group.second.front());
    if (rotation) {
      const NodeRef* driver = nullptr;
      for (const auto& ref : group.second) {
        if (same(centerOf(result, ref), rotation->sides[ref.side].centers[ref.index])) continue;
        if (!driver) driver = &ref;
        if (ref.side == (prefer_negative_seed ? 1 : 0)) {
          driver = &ref;
          break;
        }
      }
      if (driver) target = centerOf(result, *driver);
    }
    for (NodeRef ref : group.second) {
      if (anchoredAtOrigin(ref)) {
        if (rotation && !same(target, {0, 0})) {
          fail(result, targetOf(result, ref), HrnSectionFailureReason::SamePositionConstraintConflict,
               HrnSectionFailureStage::LayoutSolve);
          return false;
        }
        target = {0, 0};
      }
      else if (following_side >= 0 && ref.side != following_side)
        target = centerOf(result, ref);
    }
    shared[group.first] = target;
    for (NodeRef ref : group.second) {
      directly_mapped[globalIndex(ref)] = true;
      for (int index_of_ancestor = static_cast<int>(ref.index);
           index_of_ancestor != -1;
           index_of_ancestor = trees[ref.side].parent[index_of_ancestor])
        trees[ref.side].pinned_subtree[index_of_ancestor] = true;
    }
  }
  bool settled = false;
  for (size_t pass = 0; pass < total + 1; ++pass) {
    bool changed = false;
    for (const auto& group : groups) {
      const Point2D target = shared[group.first];
      for (NodeRef ref : group.second) {
        const Point2D current = centerOf(result, ref);
        if (same(current, target)) continue;
        if (anchoredAtOrigin(ref)) {
          fail(result, targetOf(result, ref),
               HrnSectionFailureReason::SamePositionConstraintConflict,
               HrnSectionFailureStage::LayoutSolve);
          return false;
        }
        shift(result, trees[ref.side], ref,
              {target.x - current.x, target.y - current.y});
        changed = true;
      }
    }
    if (!changed) { settled = true; break; }
  }
  if (!settled) {
    fail(result, HrnSectionTarget::positionPoint(), HrnSectionFailureReason::NonConvergent,
         HrnSectionFailureStage::LayoutSolve);
    return false;
  }
  const HrnCircularGeometryService geometry;
  std::vector<bool> rotation_followers(total, false);
  if (rotation) {
    for (const auto& group : groups) {
      for (NodeRef ref : group.second) {
        if (same(seed_centers[ref.side][ref.index], centerOf(result, ref))) continue;
        const Tree& tree = trees[ref.side];
        for (int ancestor = tree.parent[ref.index]; ancestor != -1; ancestor = tree.parent[ancestor])
          rotation_followers[globalIndex({ref.side, static_cast<size_t>(ancestor)})] = true;
      }
    }
  }
  for (int k = 0; !rotation && k < 2; ++k) {
    const auto repackMovedFamily = [&](const std::vector<size_t>& children) {
      bool mapped_moved = false;
      for (size_t child : children)
        if (trees[k].pinned_subtree[child] &&
            !same(seed_centers[k][child], result.sides[k].centers2d[child])) mapped_moved = true;
      if (mapped_moved) repackFreeSiblings(result, k, trees[k], children, classes);
    };
    for (const auto& children : trees[k].children)
      if (children.size() > 1) repackMovedFamily(children);
    if (trees[k].roots.size() > 1) repackMovedFamily(trees[k].roots);
  }
  // Jointly repair free siblings, then update and validate every parent from
  // leaves upward. Translating a pinned subtree would break a class, but a
  // parent that is not itself mapped can move around stationary children.
  size_t cycle_budget = std::numeric_limits<size_t>::max();
  for (int k = 0; k < 2; ++k) {
    const auto& side = result.sides[k];
    const Tree& tree = trees[k];
    if (tree.roots.size() > 1) {
      bool coupled_root = false;
      for (size_t root : tree.roots) coupled_root = coupled_root || tree.pinned_subtree[root];
      if (coupled_root)
        cycle_budget = std::min(cycle_budget,
            side.root->layout().solveSettings().max_iterations);
    }
    for (size_t i = 0; i < side.sections.size(); ++i)
      if (tree.pinned_subtree[i] && !tree.children[i].empty())
        cycle_budget = std::min(cycle_budget,
            side.sections[i]->layout().solveSettings().max_iterations);
  }
  if (cycle_budget == std::numeric_limits<size_t>::max()) cycle_budget = 1;
  bool jointly_settled = false;
  for (size_t cycle = 0; cycle < cycle_budget; ++cycle) {
  bool changed = false;
  for (int k = 0; k < 2; ++k) {
    HrnCircularSolvedSide& side = result.sides[k];
    Tree& tree = trees[k];
    const auto proposeRecenter = [&](NodeRef ref, HrnCircle2D& proposal) {
      const size_t id = globalIndex(ref);
      return !directly_mapped[id] &&
          (!rotation || (rotation_followers[id] && side.sections[ref.index]->autoResizeEnabled())) &&
          recenteredParent(side, tree, ref.index, geometry, proposal);
    };
    const auto repairFamily = [&](const std::vector<size_t>& children) {
      if (children.empty()) return true;
      const int parent = tree.parent[children.front()];
      const auto& container = parent == -1 ? *side.root : *side.sections[parent];
      const double spacing = container.layout().spacingTolerance();
      const int peer = 1 - k;
      bool can_repack_follower = rotation && parent == -1 && trees[peer].roots.size() > 1 &&
          normalizeDegrees(rotation->sides[peer].root_rotation) != result.sides[peer].root->rotation();
      if (rotation && !container.autoResizeEnabled()) {
        const HrnCircle2D boundary{parent == -1 ? Point2D{0, 0} : side.centers2d[parent],
                                   container.shape().dimensions().primary};
        for (size_t child : children) {
          const NodeRef member{k, child};
          const auto containment = geometry.containsWithClearance(boundary,
              {centerOf(result, member), radiusOf(result, member)}, spacing, kTolerance);
          HrnCircle2D proposal;
          if (!containment.valid || containment.contained || !proposeRecenter(member, proposal))
            continue;
          auto replacement = side.sections[child]->withCircularRadius(proposal.radius, kTolerance);
          if (!replacement) {
            fail(result, targetOf(result, member), HrnSectionFailureReason::InternalError,
                 HrnSectionFailureStage::ParentSizeUpdate);
            return false;
          }
          side.centers2d[child] = proposal.center;
          side.sections[child] = std::move(replacement);
          changed = true;
        }
      }
      bool clear = false;
      for (size_t pass = 0; pass < children.size() * children.size() + 1; ++pass) {
        bool moved = false;
        for (size_t a = 0; a < children.size(); ++a) {
          for (size_t b = a + 1; b < children.size(); ++b) {
            NodeRef left{k, children[a]}, right{k, children[b]};
            if (classes.group(globalIndex(left)) == classes.group(globalIndex(right))) continue;
            const auto distance = geometry.signedDistance(
                {centerOf(result, left), radiusOf(result, left)},
                {centerOf(result, right), radiusOf(result, right)}, kTolerance);
            if (!distance.valid) {
              fail(result, targetOf(result, left),
                   HrnSectionFailureReason::GeometryValidationFailed,
                   HrnSectionFailureStage::GeometryValidation);
              return false;
            }
            if (distance.signed_distance + kTolerance >= spacing) continue;
            const bool left_pinned = tree.pinned_subtree[left.index];
            const bool right_pinned = tree.pinned_subtree[right.index];
            if (rotation || (left_pinned && right_pinned)) {
              {
                const HrnCircle2D old_left{centerOf(result, left), radiusOf(result, left)};
                const HrnCircle2D old_right{centerOf(result, right), radiusOf(result, right)};
                HrnCircle2D new_left = old_left, new_right = old_right;
                const bool can_move_left = proposeRecenter(left, new_left);
                const bool can_move_right = proposeRecenter(right, new_right);
                std::unique_ptr<HrnSectionData> replacements[2];
                bool repaired = false;
                for (unsigned mask : {1u, 2u, 3u}) {
                  if (((mask & 1) && !can_move_left) || ((mask & 2) && !can_move_right)) continue;
                  const auto& trial_left = mask & 1 ? new_left : old_left;
                  const auto& trial_right = mask & 2 ? new_right : old_right;
                  const auto trial = geometry.signedDistance(trial_left, trial_right, kTolerance);
                  if (!trial.valid || trial.signed_distance + kTolerance < spacing) continue;
                  const NodeRef nodes[] = {left, right};
                  const HrnCircle2D circles[] = {trial_left, trial_right};
                  for (unsigned i = 0; i < 2; ++i) {
                    if (!(mask & (1u << i))) continue;
                    const auto& member = side.sections[nodes[i].index];
                    if (!member->autoResizeEnabled()) continue;
                    replacements[i] = member->withCircularRadius(circles[i].radius, kTolerance);
                    if (!replacements[i]) {
                      fail(result, targetOf(result, nodes[i]), HrnSectionFailureReason::InternalError,
                           HrnSectionFailureStage::ParentSizeUpdate);
                      return false;
                    }
                  }
                  for (unsigned i = 0; i < 2; ++i) {
                    if (!(mask & (1u << i))) continue;
                    side.centers2d[nodes[i].index] = circles[i].center;
                    if (replacements[i]) side.sections[nodes[i].index] = std::move(replacements[i]);
                  }
                  repaired = true;
                  break;
                }
                if (repaired) {
                  moved = true;
                  changed = true;
                  continue;
                }
              }
              const NodeRef obstacle = left_pinned ? left : right;
              if (can_repack_follower && left_pinned != right_pinned &&
                  (rotation_followers[globalIndex(obstacle)] ||
                   !same(seed_centers[k][obstacle.index], centerOf(result, obstacle)))) {
                can_repack_follower = false;
                if (repackFreeSiblings(result, k, tree, children, classes)) {
                  moved = true;
                  changed = true;
                  continue;
                }
              }
              fail(result, targetOf(result, left),
                   HrnSectionFailureReason::NoSolutionFound,
                   HrnSectionFailureStage::LayoutSolve);
              HrnSectionConflict conflict;
              conflict.kind = distance.signed_distance < -kTolerance ? HrnSectionConflictKind::Overlap :
                       HrnSectionConflictKind::ClearanceInsufficient;
              conflict.lhs = targetOf(result, left);
              conflict.has_rhs = true;
              conflict.rhs = targetOf(result, right);
              conflict.has_measured_value = true;
              conflict.measured_value = distance.signed_distance;
              conflict.has_required_value = true;
              conflict.required_value = spacing;
              conflict.has_lhs_witness = true;
              conflict.lhs_witness = distance.lhs_witness;
              conflict.has_rhs_witness = true;
              conflict.rhs_witness = distance.rhs_witness;
              result.status.diagnostics.back().conflicts.push_back(conflict);
              return false;
            }
            const Point2D fixed = centerOf(result, left_pinned ? left : right);
            const Point2D moving = centerOf(result, left_pinned ? right : left);
            const double dx = moving.x - fixed.x, dy = moving.y - fixed.y;
            const double length = std::hypot(dx, dy);
            const double delta = spacing - distance.signed_distance;
            const Point2D offset = length <= kTolerance ? Point2D{delta, 0} :
                Point2D{delta * dx / length, delta * dy / length};
            shift(result, tree, left_pinned ? right : left, offset);
            moved = true;
            changed = true;
          }
        }
        if (!moved) { clear = true; break; }
      }
      if (!clear) {
        fail(result, HrnSectionTarget::positionPoint(), HrnSectionFailureReason::NoSolutionFound,
             HrnSectionFailureStage::LayoutSolve);
        return false;
      }
      return true;
    };
    std::function<bool(size_t)> update = [&](size_t node) -> bool {
      for (size_t child : tree.children[node]) if (!update(child)) return false;
      if (tree.children[node].empty()) return true;
      if (rotation && !repairFamily(tree.children[node])) return false;
      const double spacing = side.sections[node]->layout().spacingTolerance();
      double required = 0;
      size_t limiting_child = tree.children[node].front();
      for (size_t child : tree.children[node]) {
        const Point2D p = side.centers2d[node], c = side.centers2d[child];
        const double extent = std::hypot(c.x - p.x, c.y - p.y) +
            side.sections[child]->shape().dimensions().primary + spacing;
        if (extent > required) { required = extent; limiting_child = child; }
      }
      if (side.sections[node]->autoResizeEnabled()) {
        if (std::fabs(side.sections[node]->shape().dimensions().primary - required) > kTolerance)
          changed = true;
        side.sections[node] = side.sections[node]->withCircularRadius(required, kTolerance);
        if (!side.sections[node]) {
          fail(result, targetOf(result, {k, node}),
               HrnSectionFailureReason::InternalError, HrnSectionFailureStage::ParentSizeUpdate);
          return false;
        }
      }
      else if (side.sections[node]->shape().dimensions().primary + kTolerance < required) {
        fail(result, targetOf(result, {k, node}),
             HrnSectionFailureReason::NoSolutionFound,
             HrnSectionFailureStage::GeometryValidation);
        addBoundaryEvidence(result, targetOf(result, {k, node}),
            targetOf(result, {k, limiting_child}), side.centers2d[node],
            side.centers2d[limiting_child],
            side.sections[limiting_child]->shape().dimensions().primary,
            side.sections[node]->shape().dimensions().primary, required);
        return false;
      }
      return side.sections[node] != nullptr;
    };
    if (!rotation) {
      for (const auto& children : tree.children)
        if (children.size() > 1 && !repairFamily(children)) return false;
      if (tree.roots.size() > 1 && !repairFamily(tree.roots)) return false;
    }
    for (size_t root : tree.roots) if (!update(root)) return false;
    if (tree.roots.size() > 1) {
      if (rotation && !repairFamily(tree.roots)) return false;
      const double spacing = side.root->layout().spacingTolerance();
      double required = 0;
      size_t limiting_root = tree.roots.front();
      for (size_t root : tree.roots) {
        const double extent = std::hypot(side.centers2d[root].x, side.centers2d[root].y) +
            side.sections[root]->shape().dimensions().primary + spacing;
        if (extent > required) { required = extent; limiting_root = root; }
      }
      if (side.root->autoResizeEnabled()) {
        if (std::fabs(side.root->shape().dimensions().primary - required) > kTolerance)
          changed = true;
        side.root = side.root->withCircularRadius(required, kTolerance);
        if (!side.root) {
          fail(result, HrnSectionTarget::sideRoot(k == 0 ? ZeHrnSide::Positive :
               ZeHrnSide::Negative), HrnSectionFailureReason::InternalError,
               HrnSectionFailureStage::ParentSizeUpdate);
          return false;
        }
      }
      else if (side.root->shape().dimensions().primary + kTolerance < required) {
        fail(result, HrnSectionTarget::sideRoot(k == 0 ? ZeHrnSide::Positive :
             ZeHrnSide::Negative), HrnSectionFailureReason::NoSolutionFound,
             HrnSectionFailureStage::GeometryValidation);
        addBoundaryEvidence(result, HrnSectionTarget::sideRoot(k == 0 ?
            ZeHrnSide::Positive : ZeHrnSide::Negative),
            targetOf(result, {k, limiting_root}), {0, 0},
            side.centers2d[limiting_root],
            side.sections[limiting_root]->shape().dimensions().primary,
            side.root->shape().dimensions().primary, required);
        return false;
      }
    }
  }
  if (rotation || !changed) { jointly_settled = true; break; }
  }
  if (!jointly_settled) {
    fail(result, HrnSectionTarget::positionPoint(),
         HrnSectionFailureReason::IterationBudgetExceeded,
         HrnSectionFailureStage::LayoutSolve);
    result.status.diagnostics.back().iterations_used = cycle_budget;
    return false;
  }
  for (int k = 0; k < 2; ++k) {
    HrnCircularSolvedSide& side = result.sides[k];
    const Tree& tree = trees[k];
    for (size_t parent = 0; parent < side.segments.size(); ++parent) {
      const auto& children = tree.children[parent];
      if (children.empty()) continue;
      const HrnCircle2D parent_circle{side.centers2d[parent],
          side.sections[parent]->shape().dimensions().primary};
      const double spacing = side.sections[parent]->layout().spacingTolerance();
      for (size_t child : children) {
        const auto check = geometry.containsWithClearance(parent_circle,
            {side.centers2d[child], side.sections[child]->shape().dimensions().primary},
            spacing, kTolerance);
        if (!check.valid || !check.contained) {
          fail(result, targetOf(result, {k, parent}),
               HrnSectionFailureReason::GeometryValidationFailed,
               HrnSectionFailureStage::GeometryValidation);
          return false;
        }
      }
    }
    for (size_t parent = 0; parent <= side.segments.size(); ++parent) {
      const bool root = parent == side.segments.size();
      const auto& children = root ? tree.roots : tree.children[parent];
      if (root && children.size() <= 1) continue;
      const double spacing = root ? side.root->layout().spacingTolerance() :
          side.sections[parent]->layout().spacingTolerance();
      if (root) {
        for (size_t child : children) {
          const auto check = geometry.containsWithClearance(
              {{0, 0}, side.root->shape().dimensions().primary},
              {side.centers2d[child], side.sections[child]->shape().dimensions().primary},
              spacing, kTolerance);
          if (!check.valid || !check.contained) {
            fail(result, HrnSectionTarget::sideRoot(k == 0 ? ZeHrnSide::Positive :
                 ZeHrnSide::Negative), HrnSectionFailureReason::GeometryValidationFailed,
                 HrnSectionFailureStage::GeometryValidation);
            return false;
          }
        }
      }
      for (size_t i = 0; i < children.size(); ++i)
        for (size_t j = i + 1; j < children.size(); ++j) {
          if (classes.group(globalIndex({k, children[i]})) ==
              classes.group(globalIndex({k, children[j]}))) continue;
          const auto distance = geometry.signedDistance(
              {side.centers2d[children[i]], side.sections[children[i]]->shape().dimensions().primary},
              {side.centers2d[children[j]], side.sections[children[j]]->shape().dimensions().primary},
              kTolerance);
          if (!distance.valid || distance.signed_distance + kTolerance < spacing) {
            fail(result, root ? HrnSectionTarget::sideRoot(k == 0 ? ZeHrnSide::Positive :
                 ZeHrnSide::Negative) : targetOf(result, {k, parent}),
                 HrnSectionFailureReason::GeometryValidationFailed,
                 HrnSectionFailureStage::GeometryValidation);
            return false;
          }
        }
    }
    for (size_t i = 0; i < side.segments.size(); ++i)
      side.locations[i] = transformPoint(input.positionTransform(), side.centers2d[i]);
  }
  for (const auto& group : groups)
    for (NodeRef ref : group.second)
      if (!same(centerOf(result, ref), shared[group.first])) {
        fail(result, targetOf(result, ref),
             HrnSectionFailureReason::GeometryValidationFailed,
             HrnSectionFailureStage::GeometryValidation);
        return false;
      }
  return true;
}
}  // namespace hrn
