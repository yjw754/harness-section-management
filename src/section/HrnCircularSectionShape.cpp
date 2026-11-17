#include <cmath>

#include "hrn/section/HrnSectionShape.h"
namespace hrn {
std::unique_ptr<HrnCircularSectionShape> HrnCircularSectionShape::create(double r, double t) {
  if (!std::isfinite(r) || !std::isfinite(t) || t <= 0 || r <= t)
    return std::unique_ptr<HrnCircularSectionShape>();
  return std::unique_ptr<HrnCircularSectionShape>(new HrnCircularSectionShape(r));
}
std::unique_ptr<HrnSectionShape> HrnCircularSectionShape::clone() const {
  return std::unique_ptr<HrnSectionShape>(new HrnCircularSectionShape(radius_));
}
}  // namespace hrn
