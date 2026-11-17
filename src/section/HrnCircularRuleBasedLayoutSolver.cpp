#include "hrn/section/HrnRuleBasedLayoutSolver.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

#include "hrn/section/HrnSectionGeometryService.h"

namespace hrn {
namespace {
const double kGeometryTolerance = 1e-9;
double envelopeRadius(const HrnCircularLayoutPlacement& placement, double spacing) {
  return std::hypot(placement.center.x, placement.center.y) + placement.radius + spacing;
}
bool validCandidate(const Point2D& point, double radius,
                    const std::vector<HrnCircularLayoutPlacement>& placed,
                    double spacing, const HrnCircularGeometryService& geometry) {
  if (!std::isfinite(point.x) || !std::isfinite(point.y)) return false;
  const HrnCircle2D circle{point, radius};
  for (const auto& previous : placed) {
    const auto distance = geometry.signedDistance(circle,
        HrnCircle2D{previous.center, previous.radius}, kGeometryTolerance);
    if (!distance.valid || distance.signed_distance + kGeometryTolerance < spacing) return false;
  }
  return true;
}
bool better(double score, double best_score) {
  // Pair order, then upper intersection, then compass fallback decide equal scores.
  return score < best_score - kGeometryTolerance;
}
}  // namespace

HrnRuleLayoutResult HrnCircularRuleBasedLayoutSolver::solve(
    const HrnCircularLayoutRequest& request) const {
  HrnRuleLayoutResult result;
  if (!std::isfinite(request.spacing) || request.spacing < 0 ||
      !request.settings.valid() ||
      request.settings.algorithm_version != kAlgorithmVersion) {
    result.reason = HrnSectionFailureReason::InvalidInput;
    return result;
  }
  std::set<size_t> ids;
  for (const auto& member : request.members) {
    if (!std::isfinite(member.radius) || member.radius <= 0 || !ids.insert(member.id).second) {
      result.reason = HrnSectionFailureReason::InvalidInput;
      return result;
    }
  }
  std::vector<HrnCircularLayoutMember> sorted = request.members;
  std::stable_sort(sorted.begin(), sorted.end(), [](const HrnCircularLayoutMember& a,
                                                     const HrnCircularLayoutMember& b) {
    return a.radius > b.radius;
  });
  const HrnCircularGeometryService geometry;
  for (const auto& member : sorted) {
    if (result.iterations_used == request.settings.max_iterations) {
      result.reason = HrnSectionFailureReason::IterationBudgetExceeded;
      return result;
    }
    ++result.iterations_used;
    Point2D chosen{0, 0};
    bool found = false;
    double best_score = std::numeric_limits<double>::infinity();
    const auto consider = [&](const Point2D& point) {
      ++result.candidate_count;
      if (!validCandidate(point, member.radius, result.placements, request.spacing, geometry))
        return;
      const double score = std::max(result.required_parent_radius,
          envelopeRadius(HrnCircularLayoutPlacement{member.id, member.radius, point},
                         request.spacing));
      if (!std::isfinite(score)) return;
      if (!found || better(score, best_score)) {
        chosen = point;
        best_score = score;
        found = true;
      }
    };
    if (result.placements.empty()) {
      consider({0, 0});
    } else if (result.placements.size() == 1) {
      const auto& first = result.placements.front();
      consider({first.center.x + first.radius + member.radius + request.spacing,
                first.center.y});
    } else {
      for (size_t i = 0; i < result.placements.size(); ++i) {
        for (size_t j = i + 1; j < result.placements.size(); ++j) {
          const auto& a = result.placements[i];
          const auto& b = result.placements[j];
          const double dx = b.center.x - a.center.x;
          const double dy = b.center.y - a.center.y;
          const double d = std::hypot(dx, dy);
          const double ra = a.radius + member.radius + request.spacing;
          const double rb = b.radius + member.radius + request.spacing;
          if (d <= kGeometryTolerance || d > ra + rb + kGeometryTolerance ||
              d < std::fabs(ra - rb) - kGeometryTolerance) continue;
          const double along = (ra * ra - rb * rb + d * d) / (2 * d);
          const double h2 = ra * ra - along * along;
          if (h2 < -kGeometryTolerance) continue;
          const double h = std::sqrt(std::max(0.0, h2));
          const double mx = a.center.x + along * dx / d;
          const double my = a.center.y + along * dy / d;
          consider({mx - h * dy / d, my + h * dx / d});
          consider({mx + h * dy / d, my - h * dx / d});
        }
      }
      // A single tangent also supplies candidates when no pair can support a new circle.
      const double diagonal = std::sqrt(0.5);
      const Point2D directions[] = {{1, 0}, {diagonal, diagonal}, {0, 1},
          {-diagonal, diagonal}, {-1, 0}, {-diagonal, -diagonal},
          {0, -1}, {diagonal, -diagonal}};
      for (const auto& previous : result.placements) {
        const double distance = previous.radius + member.radius + request.spacing;
        for (const auto& direction : directions)
          consider({previous.center.x + distance * direction.x,
                    previous.center.y + distance * direction.y});
      }
    }
    if (!found) {
      result.reason = HrnSectionFailureReason::NoSolutionFound;
      return result;
    }
    result.placements.push_back({member.id, member.radius, chosen});
    result.required_parent_radius = best_score;
  }
  // Independent circular geometry recheck before reporting success.
  for (size_t i = 0; i < result.placements.size(); ++i) {
    const auto& a = result.placements[i];
    const auto containment = geometry.containsWithClearance(
        HrnCircle2D{{0, 0}, result.required_parent_radius},
        HrnCircle2D{a.center, a.radius}, request.spacing, kGeometryTolerance);
    if (!containment.valid || !containment.contained) {
      result.reason = HrnSectionFailureReason::GeometryValidationFailed;
      return result;
    }
    for (size_t j = i + 1; j < result.placements.size(); ++j) {
      const auto& b = result.placements[j];
      const auto distance = geometry.signedDistance(
          HrnCircle2D{a.center, a.radius}, HrnCircle2D{b.center, b.radius},
          kGeometryTolerance);
      if (!distance.valid || distance.signed_distance + kGeometryTolerance < request.spacing) {
        result.reason = HrnSectionFailureReason::GeometryValidationFailed;
        return result;
      }
    }
  }
  result.succeeded = true;
  return result;
}
}  // namespace hrn
