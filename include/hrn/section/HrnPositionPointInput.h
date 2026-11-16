#pragma once
#include "hrn/section/HrnGeometryTypes.h"
#include "hrn/section/HrnSectionData.h"
#include "hrn/section/HrnSegment.h"
#include <vector>
namespace hrn {
class HrnPositionPointInput {
public:
 static std::unique_ptr<HrnPositionPointInput> create(const Matrix4D& transform,std::unique_ptr<HrnSectionData> positive_root,std::unique_ptr<HrnSectionData> negative_root);
 bool append(ZeHrnSide side,HrnSegment& segment,std::unique_ptr<HrnSectionData> section);
 const Matrix4D& positionTransform() const { return transform_; }
 size_t sectionCount(ZeHrnSide side) const;
 HrnSegment& segmentAt(ZeHrnSide side,size_t index) const;
 const HrnSectionData& sectionAt(ZeHrnSide side,size_t index) const;
 const HrnSectionData& sideRootSection(ZeHrnSide side) const;
 const HrnSectionData* getEffectiveRootSection(ZeHrnSide side) const;
 bool findSectionIndex(ZeHrnSide side,const HrnSegment& segment,size_t& index) const;
private:
 struct SideData { std::vector<HrnSegment*> segments; std::vector<std::unique_ptr<HrnSectionData> > sections; std::unique_ptr<HrnSectionData> root; };
 HrnPositionPointInput(const Matrix4D& transform,std::unique_ptr<HrnSectionData> positive_root,std::unique_ptr<HrnSectionData> negative_root);
 const SideData& side(ZeHrnSide value) const { return sides_[value==ZeHrnSide::Positive?0:1]; }
 SideData& side(ZeHrnSide value) { return sides_[value==ZeHrnSide::Positive?0:1]; }
 Matrix4D transform_;
 SideData sides_[2];
};
}
