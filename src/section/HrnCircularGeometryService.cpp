#include "hrn/section/HrnSectionGeometryService.h"

#include <cmath>

namespace hrn {
namespace {
bool validCircle(const HrnCircle2D& circle) {
  return std::isfinite(circle.center.x) && std::isfinite(circle.center.y) &&
         std::isfinite(circle.radius) && circle.radius > 0;
}
bool validTolerance(double tolerance) {
  return std::isfinite(tolerance) && tolerance >= 0;
}
Point2D direction(const Point2D& from, const Point2D& to) {
  const double dx = to.x - from.x;
  const double dy = to.y - from.y;
  const double length = std::hypot(dx, dy);
  return length == 0 ? Point2D{1, 0} : Point2D{dx / length, dy / length};
}
Point2D along(const Point2D& origin, const Point2D& axis, double distance) {
  return {origin.x + distance * axis.x, origin.y + distance * axis.y};
}
}  // namespace

HrnContourDistanceResult HrnCircularGeometryService::signedDistance(
    const HrnCircle2D& lhs, const HrnCircle2D& rhs, double tolerance) const {
  HrnContourDistanceResult result;
  if (!validCircle(lhs) || !validCircle(rhs) || !validTolerance(tolerance)) return result;
  const Point2D axis = direction(lhs.center, rhs.center);
  result.signed_distance = std::hypot(rhs.center.x - lhs.center.x,
                                      rhs.center.y - lhs.center.y) - lhs.radius - rhs.radius;
  if (!std::isfinite(result.signed_distance)) return result;
  result.lhs_witness = along(lhs.center, axis, lhs.radius);
  result.rhs_witness = along(rhs.center, axis, -rhs.radius);
  result.valid = true;
  return result;
}

HrnContainmentResult HrnCircularGeometryService::containsWithClearance(
    const HrnCircle2D& parent, const HrnCircle2D& child, double clearance,
    double tolerance) const {
  HrnContainmentResult result;
  if (!validCircle(parent) || !validCircle(child) || !validTolerance(clearance) ||
      !validTolerance(tolerance)) return result;
  const Point2D axis = direction(parent.center, child.center);
  result.minimum_clearance = parent.radius -
      std::hypot(child.center.x - parent.center.x, child.center.y - parent.center.y) -
      child.radius;
  if (!std::isfinite(result.minimum_clearance)) return result;
  result.parent_witness = along(parent.center, axis, parent.radius);
  result.child_witness = along(child.center, axis, child.radius);
  result.contained = result.minimum_clearance + tolerance >= clearance;
  result.valid = true;
  return result;
}

bool HrnCircularGeometryService::intersects(const HrnCircle2D& lhs, const HrnCircle2D& rhs,
                                            double tolerance) const {
  const HrnContourDistanceResult distance = signedDistance(lhs, rhs, tolerance);
  return distance.valid && distance.signed_distance <= tolerance;
}
}  // namespace hrn
