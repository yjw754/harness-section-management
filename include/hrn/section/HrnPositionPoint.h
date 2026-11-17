#pragma once
#include "hrn/section/HrnPositionPointInput.h"
#include "hrn/section/HrnRootSectionDefaultProvider.h"
namespace hrn {
class HrnPositionPointSolver;
class HrnPositionPointTransaction;
class HrnPositionPoint {
 public:
  static std::unique_ptr<HrnPositionPoint> createEmpty(
      const Matrix4D& transform, std::unique_ptr<HrnSectionData> positive_root,
      std::unique_ptr<HrnSectionData> negative_root);
  static std::unique_ptr<HrnPositionPoint> createEmpty(
      const Matrix4D& transform, const HrnRootSectionDefaultProvider& provider);
  std::unique_ptr<HrnPositionPoint> clone() const;
  const Matrix4D& positionTransform() const { return transform_; }
  Point3D center() const { return hrn::center(transform_); }
  size_t sectionCount(ZeHrnSide side) const;
  HrnSegment& segmentAt(ZeHrnSide side, size_t index) const;
  Point3D locationAt(ZeHrnSide side, size_t index) const;
  const HrnSectionData& sectionAt(ZeHrnSide side, size_t index) const;
  const HrnSectionData& sideRootSection(ZeHrnSide side) const;
  const HrnSectionData* getEffectiveRootSection(ZeHrnSide side) const;
  bool findSectionIndex(ZeHrnSide side, const HrnSegment& segment, size_t& index) const;
  static bool validateSectionData(const HrnSectionData& section);

 private:
  struct SideData {
    std::vector<HrnSegment*> segments;
    std::vector<Point3D> locations;
    std::vector<std::unique_ptr<HrnSectionData>> sections;
    std::unique_ptr<HrnSectionData> root;
  };
  HrnPositionPoint(const Matrix4D& transform, std::unique_ptr<HrnSectionData> positive_root,
                   std::unique_ptr<HrnSectionData> negative_root);
  const SideData& side(ZeHrnSide value) const {
    if (value != ZeHrnSide::Positive && value != ZeHrnSide::Negative)
      throw std::invalid_argument("invalid side");
    return sides_[value == ZeHrnSide::Positive ? 0 : 1];
  }
  Matrix4D transform_;
  SideData sides_[2];
  friend class HrnPositionPointSolver;
  friend class HrnPositionPointTransaction;
};
}  // namespace hrn
