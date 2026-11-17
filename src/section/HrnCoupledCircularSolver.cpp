#include "hrn/section/HrnPositionPointSolver.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <vector>

#include "hrn/section/HrnSectionGeometryService.h"

namespace hrn {
namespace {
const double kTolerance = 1e-9;
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
size_t find(std::vector<size_t>& leaders, size_t value) {
  while (leaders[value] != value) {
    leaders[value] = leaders[leaders[value]];
    value = leaders[value];
  }
  return value;
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
}  // namespace

bool enforceCoupledCircular(const HrnPositionPointInput& input,
                            const std::vector<HrnSegmentSideRelation>& relations,
                            HrnCircularHierarchyResult& result) {
  const size_t positive_count = result.sides[0].segments.size();
  const size_t negative_count = result.sides[1].segments.size();
  const size_t total = positive_count + negative_count;
  std::vector<size_t> leaders(total);
  for (size_t i = 0; i < total; ++i) leaders[i] = i;
  std::map<const HrnSegment*, size_t> index[2];
  for (size_t i = 0; i < positive_count; ++i) index[0][result.sides[0].segments[i]] = i;
  for (size_t i = 0; i < negative_count; ++i) index[1][result.sides[1].segments[i]] = i;
  bool coupled = false;
  for (const auto& relation : relations) {
    if (!relation.valid() || index[0].count(relation.positive) == 0 ||
        index[1].count(relation.negative) == 0) {
      fail(result, HrnSectionTarget::positionPoint(), HrnSectionFailureReason::InvalidInput,
           HrnSectionFailureStage::InputValidation);
      return false;
    }
    if (!relation.same_position) continue;
    coupled = true;
    const size_t a = find(leaders, index[0][relation.positive]);
    const size_t b = find(leaders, positive_count + index[1][relation.negative]);
    if (a != b) leaders[b] = a;
  }
  if (!coupled) return true;
  Tree trees[2] = {makeTree(result.sides[0]), makeTree(result.sides[1])};
  std::map<size_t, std::vector<NodeRef>> groups;
  for (size_t i = 0; i < total; ++i) {
    const size_t leader = find(leaders, i);
    if (leader == i &&
        ((i < positive_count && index[0].empty()) ||
         (i >= positive_count && index[1].empty()))) continue;
    groups[leader].push_back(i < positive_count ? NodeRef{0, i} :
        NodeRef{1, i - positive_count});
  }
  for (auto it = groups.begin(); it != groups.end();) {
    if (it->second.size() < 2) it = groups.erase(it);
    else ++it;
  }
  // A shared center is chosen once per connected component. A lone side root
  // anchors its component at the Position Point origin.
  std::map<size_t, Point2D> shared;
  for (const auto& group : groups) {
    Point2D target = centerOf(result, group.second.front());
    for (NodeRef ref : group.second)
      if (trees[ref.side].roots.size() == 1 &&
          trees[ref.side].parent[ref.index] == -1) target = {0, 0};
    shared[group.first] = target;
    for (NodeRef ref : group.second) {
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
        if (trees[ref.side].roots.size() == 1 &&
            trees[ref.side].parent[ref.index] == -1) {
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
  // Jointly repair free siblings, then update and validate every parent from
  // leaves upward. A pinned subtree cannot be moved without breaking a class.
  size_t cycle_budget = 1000;
  for (int k = 0; k < 2; ++k) {
    const auto& side = result.sides[k];
    cycle_budget = std::min(cycle_budget, side.root->layout().solveSettings().max_iterations);
    for (const auto& section : side.sections)
      cycle_budget = std::min(cycle_budget, section->layout().solveSettings().max_iterations);
  }
  bool jointly_settled = false;
  for (size_t cycle = 0; cycle < cycle_budget; ++cycle) {
  bool changed = false;
  for (int k = 0; k < 2; ++k) {
    HrnCircularSolvedSide& side = result.sides[k];
    Tree& tree = trees[k];
    std::vector<std::vector<size_t>> families;
    for (const auto& children : tree.children) if (!children.empty()) families.push_back(children);
    if (tree.roots.size() > 1) families.push_back(tree.roots);
    for (const auto& children : families) {
      bool clear = false;
      for (size_t pass = 0; pass < children.size() * children.size() + 1; ++pass) {
        bool moved = false;
        for (size_t a = 0; a < children.size(); ++a) {
          for (size_t b = a + 1; b < children.size(); ++b) {
            NodeRef left{k, children[a]}, right{k, children[b]};
            const double spacing = tree.parent[children[a]] == -1 ?
                side.root->layout().spacingTolerance() :
                side.sections[tree.parent[children[a]]]->layout().spacingTolerance();
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
            if (left_pinned && right_pinned) {
              fail(result, targetOf(result, left),
                   HrnSectionFailureReason::SamePositionConstraintConflict,
                   HrnSectionFailureStage::LayoutSolve);
              HrnSectionConflict conflict;
              conflict.kind = HrnSectionConflictKind::SamePositionIncompatible;
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
    }
    std::function<bool(size_t)> update = [&](size_t node) -> bool {
      for (size_t child : tree.children[node]) if (!update(child)) return false;
      if (tree.children[node].empty()) return true;
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
        bool all_pinned = true;
        for (size_t child : tree.children[node])
          all_pinned = all_pinned && tree.pinned_subtree[child];
        const bool anchored = tree.roots.size() == 1 && tree.parent[node] == -1;
        fail(result, targetOf(result, {k, node}),
             anchored && all_pinned ?
                 HrnSectionFailureReason::SamePositionConstraintConflict :
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
    for (size_t root : tree.roots) if (!update(root)) return false;
    if (tree.roots.size() > 1) {
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
        bool all_pinned = true;
        for (size_t root : tree.roots)
          all_pinned = all_pinned && tree.pinned_subtree[root];
        fail(result, HrnSectionTarget::sideRoot(k == 0 ? ZeHrnSide::Positive :
             ZeHrnSide::Negative), all_pinned ?
                 HrnSectionFailureReason::SamePositionConstraintConflict :
                 HrnSectionFailureReason::NoSolutionFound,
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
  if (!changed) { jointly_settled = true; break; }
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
