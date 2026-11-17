#include "hrn/section/HrnSectionGeometryService.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace hrn {
namespace {
bool valid(const HrnCircle2D& circle) {
  return std::isfinite(circle.center.x) && std::isfinite(circle.center.y) &&
         std::isfinite(circle.radius) && circle.radius > 0;
}
bool covers(const HrnCircle2D& outer, const HrnCircle2D& inner, double tolerance) {
  return std::hypot(outer.center.x - inner.center.x,
                    outer.center.y - inner.center.y) + inner.radius <= outer.radius + tolerance;
}
HrnCircle2D two(const HrnCircle2D& a, const HrnCircle2D& b) {
  const double dx = b.center.x - a.center.x;
  const double dy = b.center.y - a.center.y;
  const double distance = std::hypot(dx, dy);
  if (distance + b.radius <= a.radius) return a;
  if (distance + a.radius <= b.radius) return b;
  const double radius = (distance + a.radius + b.radius) / 2;
  const double ratio = (radius - a.radius) / distance;
  return {{a.center.x + ratio * dx, a.center.y + ratio * dy}, radius};
}
HrnMinimumEnclosingCircleResult three(const HrnCircle2D& p,
                                       const HrnCircle2D& q,
                                       const HrnCircle2D& r,
                                       double tolerance) {
  HrnMinimumEnclosingCircleResult best;
  const HrnCircle2D disks[] = {p, q, r};
  const auto consider = [&](HrnCircle2D candidate) {
    if (!std::isfinite(candidate.center.x) || !std::isfinite(candidate.center.y) ||
        !std::isfinite(candidate.radius)) return;
    for (const auto& disk : disks)
      if (!covers(candidate, disk, tolerance)) return;
    if (!best.valid || candidate.radius < best.circle.radius) {
      best.circle = candidate;
      best.valid = true;
    }
  };
  for (const auto& disk : disks) consider(disk);
  consider(two(p, q));
  consider(two(p, r));
  consider(two(q, r));
  if (best.valid) return best;

  // External tangent circle: center = A + B * radius from pairwise equations.
  const double ux = q.center.x - p.center.x, uy = q.center.y - p.center.y;
  const double vx = r.center.x - p.center.x, vy = r.center.y - p.center.y;
  const double determinant = ux * vy - uy * vx;
  if (std::fabs(determinant) <= tolerance) return best;
  const double cq = (q.center.x * q.center.x + q.center.y * q.center.y -
      p.center.x * p.center.x - p.center.y * p.center.y -
      q.radius * q.radius + p.radius * p.radius) / 2;
  const double cr = (r.center.x * r.center.x + r.center.y * r.center.y -
      p.center.x * p.center.x - p.center.y * p.center.y -
      r.radius * r.radius + p.radius * p.radius) / 2;
  const double bq = q.radius - p.radius, br = r.radius - p.radius;
  const Point2D a{(cq * vy - uy * cr) / determinant,
                  (ux * cr - cq * vx) / determinant};
  const Point2D b{(bq * vy - uy * br) / determinant,
                  (ux * br - bq * vx) / determinant};
  const double ax = a.x - p.center.x, ay = a.y - p.center.y;
  const double quadratic = b.x * b.x + b.y * b.y - 1;
  const double linear = 2 * (ax * b.x + ay * b.y + p.radius);
  const double constant = ax * ax + ay * ay - p.radius * p.radius;
  const auto considerRoot = [&](double radius) {
    if (radius < p.radius || radius < q.radius || radius < r.radius) return;
    consider({{a.x + b.x * radius, a.y + b.y * radius}, radius});
  };
  if (std::fabs(quadratic) <= tolerance) {
    if (std::fabs(linear) > tolerance) considerRoot(-constant / linear);
  } else {
    const double discriminant = linear * linear - 4 * quadratic * constant;
    if (discriminant >= -tolerance) {
      const double root = std::sqrt(std::max(0.0, discriminant));
      considerRoot((-linear - root) / (2 * quadratic));
      considerRoot((-linear + root) / (2 * quadratic));
    }
  }
  return best;
}
}  // namespace

HrnMinimumEnclosingCircleResult HrnCircularGeometryService::minimumEnclosingCircle(
    const std::vector<HrnCircle2D>& circles, double tolerance) const {
  HrnMinimumEnclosingCircleResult result;
  if (circles.empty() || !std::isfinite(tolerance) || tolerance < 0) return result;
  for (const auto& disk : circles) if (!valid(disk)) return result;
  result.circle = circles.front();
  result.valid = true;
  for (size_t i = 1; i < circles.size(); ++i) {
    if (covers(result.circle, circles[i], tolerance)) continue;
    result.circle = circles[i];
    for (size_t j = 0; j < i; ++j) {
      if (covers(result.circle, circles[j], tolerance)) continue;
      result.circle = two(circles[i], circles[j]);
      for (size_t k = 0; k < j; ++k) {
        if (covers(result.circle, circles[k], tolerance)) continue;
        const auto candidate = three(circles[i], circles[j], circles[k], tolerance);
        if (!candidate.valid) return HrnMinimumEnclosingCircleResult();
        result.circle = candidate.circle;
      }
    }
  }
  for (const auto& disk : circles)
    if (!covers(result.circle, disk, tolerance)) return HrnMinimumEnclosingCircleResult();
  return result;
}
}  // namespace hrn
