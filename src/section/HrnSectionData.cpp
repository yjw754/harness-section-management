#include "hrn/section/HrnSectionData.h"

#include <cmath>
namespace hrn {
HrnSectionData::HrnSectionData(std::unique_ptr<HrnSectionShape> s,
                               std::unique_ptr<HrnSectionLayout> l, const HrnSectionOptions& o)
    : shape_(std::move(s)), layout_(std::move(l)), options_(o) {
  options_.rotation = normalizeDegrees(o.rotation);
}
std::unique_ptr<HrnSectionData> HrnSectionData::create(std::unique_ptr<HrnSectionShape> s,
                                                       std::unique_ptr<HrnSectionLayout> l,
                                                       const HrnSectionOptions& o) {
  if (!s || !l || !std::isfinite(o.rotation) || !l->solveSettings().valid() ||
      !std::isfinite(l->spacingTolerance()) || l->spacingTolerance() < 0)
    return std::unique_ptr<HrnSectionData>();
  if (o.size_update_mode != HrnSectionSizeUpdateMode::KeepRatioConstant &&
      o.size_update_mode != HrnSectionSizeUpdateMode::KeepSmallestValue &&
      o.size_update_mode != HrnSectionSizeUpdateMode::KeepGreatestValue)
    return std::unique_ptr<HrnSectionData>();
  const HrnSectionDimensions d = s->dimensions();
  if (d.shape_type != ZeHrnSectionShapeType::Circular || !std::isfinite(d.primary) ||
      d.primary <= 0)
    return std::unique_ptr<HrnSectionData>();
  return std::unique_ptr<HrnSectionData>(new HrnSectionData(std::move(s), std::move(l), o));
}
std::unique_ptr<HrnSectionData> HrnSectionData::clone() const {
  return create(shape_->clone(), layout_->clone(), options_);
}
std::unique_ptr<HrnSectionData> HrnSectionData::withCircularRadius(
    double radius, double modeling_tolerance) const {
  return create(HrnCircularSectionShape::create(radius, modeling_tolerance),
                layout_->clone(), options_);
}
bool HrnSectionData::setRotation(double r) {
  if (!std::isfinite(r)) return false;
  options_.rotation = normalizeDegrees(r);
  return true;
}
}  // namespace hrn
