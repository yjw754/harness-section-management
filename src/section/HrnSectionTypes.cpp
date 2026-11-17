#include "hrn/section/HrnSectionTypes.h"

#include <cmath>
namespace hrn {
double normalizeDegrees(double d) {
  if (!std::isfinite(d)) return d;
  double result = std::fmod(d, 360.0);
  if (result < 0) result += 360.0;
  return result >= 360.0 ? 0 : result;
}
}  // namespace hrn
