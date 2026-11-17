#include "hrn/section/HrnPositionPoint.h"

#include <algorithm>
#include <cmath>
namespace hrn {
HrnPositionPoint::HrnPositionPoint(const Matrix4D& m, std::unique_ptr<HrnSectionData> p,
                                   std::unique_ptr<HrnSectionData> n)
    : transform_(m) {
  sides_[0].root = std::move(p);
  sides_[1].root = std::move(n);
}
bool HrnPositionPoint::validateSectionData(const HrnSectionData& section) {
  const HrnSectionDimensions d = section.shape().dimensions();
  return d.shape_type == ZeHrnSectionShapeType::Circular && std::isfinite(d.primary) &&
         d.primary > 0 && std::isfinite(section.rotation()) &&
         section.layout().type() == ZeHrnSectionLayoutType::RuleBased &&
         section.layout().solveSettings().valid() &&
         std::isfinite(section.layout().spacingTolerance()) &&
         section.layout().spacingTolerance() >= 0 &&
         (section.sizeUpdateMode() == HrnSectionSizeUpdateMode::KeepRatioConstant ||
          section.sizeUpdateMode() == HrnSectionSizeUpdateMode::KeepSmallestValue ||
          section.sizeUpdateMode() == HrnSectionSizeUpdateMode::KeepGreatestValue);
}
std::unique_ptr<HrnPositionPoint> HrnPositionPoint::createEmpty(const Matrix4D& m,
                                                                std::unique_ptr<HrnSectionData> p,
                                                                std::unique_ptr<HrnSectionData> n) {
  if (p && p.get() == n.get()) {
    n.release();
    return std::unique_ptr<HrnPositionPoint>();
  }
  if (!isRigidTransform(m, 1e-9) || !p || !n || !validateSectionData(*p) ||
      !validateSectionData(*n))
    return std::unique_ptr<HrnPositionPoint>();
  return std::unique_ptr<HrnPositionPoint>(new HrnPositionPoint(m, std::move(p), std::move(n)));
}
std::unique_ptr<HrnPositionPoint> HrnPositionPoint::createEmpty(
    const Matrix4D& m, const HrnRootSectionDefaultProvider& p) {
  return createEmpty(m, p.create(ZeHrnSide::Positive), p.create(ZeHrnSide::Negative));
}
std::unique_ptr<HrnPositionPoint> HrnPositionPoint::clone() const {
  std::unique_ptr<HrnPositionPoint> result =
      createEmpty(transform_, sides_[0].root->clone(), sides_[1].root->clone());
  if (!result) return result;
  for (int k = 0; k < 2; ++k) {
    result->sides_[k].segments = sides_[k].segments;
    result->sides_[k].locations = sides_[k].locations;
    for (const auto& section : sides_[k].sections)
      result->sides_[k].sections.push_back(section->clone());
  }
  result->relations_ = relations_;
  return result;
}
size_t HrnPositionPoint::sectionCount(ZeHrnSide s) const {
  return side(s).sections.size();
}
HrnSegment& HrnPositionPoint::segmentAt(ZeHrnSide s, size_t i) const {
  return *side(s).segments.at(i);
}
Point3D HrnPositionPoint::locationAt(ZeHrnSide s, size_t i) const {
  return side(s).locations.at(i);
}
const HrnSectionData& HrnPositionPoint::sectionAt(ZeHrnSide s, size_t i) const {
  return *side(s).sections.at(i);
}
const HrnSectionData& HrnPositionPoint::sideRootSection(ZeHrnSide s) const {
  return *side(s).root;
}
bool HrnPositionPoint::findSectionIndex(ZeHrnSide s, const HrnSegment& segment,
                                        size_t& index) const {
  const auto& entries = side(s).segments;
  auto it = std::find(entries.begin(), entries.end(), &segment);
  if (it == entries.end()) return false;
  index = static_cast<size_t>(it - entries.begin());
  return true;
}
const HrnSectionData* HrnPositionPoint::getEffectiveRootSection(ZeHrnSide s) const {
  const auto& d = side(s);
  size_t count = 0, index = 0;
  for (size_t i = 0; i < d.segments.size(); ++i)
    if (std::find(d.segments.begin(), d.segments.end(), d.segments[i]->parent()) ==
        d.segments.end()) {
      ++count;
      index = i;
    }
  return count == 0 ? 0 : (count == 1 ? d.sections[index].get() : d.root.get());
}
}  // namespace hrn
