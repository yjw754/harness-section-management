#pragma once
#include "hrn/section/HrnGeometryTypes.h"
namespace hrn {
enum class ZeHrnSide { Positive, Negative };
enum class ZeHrnSectionShapeType { Circular, Elliptical, Rectangular };
enum class ZeHrnSectionLayoutType { RuleBased };
enum class HrnSectionSizeUpdateMode { KeepRatioConstant, KeepSmallestValue, KeepGreatestValue };
struct HrnSectionDimensions {
  ZeHrnSectionShapeType shape_type;
  double primary, secondary;
  static HrnSectionDimensions circle(double radius) {
    return {ZeHrnSectionShapeType::Circular, radius, radius};
  }
};
inline bool operator==(const HrnSectionDimensions& a, const HrnSectionDimensions& b) {
  return a.shape_type == b.shape_type && a.primary == b.primary && a.secondary == b.secondary;
}
double normalizeDegrees(double degrees);
}  // namespace hrn
