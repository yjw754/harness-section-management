#include "hrn/section/HrnPositionPointInput.h"
#include <algorithm>
namespace hrn {
HrnPositionPointInput::HrnPositionPointInput(const Matrix4D& m,std::unique_ptr<HrnSectionData> p,std::unique_ptr<HrnSectionData> n) : transform_(m) { sides_[0].root=std::move(p); sides_[1].root=std::move(n); }
std::unique_ptr<HrnPositionPointInput> HrnPositionPointInput::create(const Matrix4D& m,std::unique_ptr<HrnSectionData> p,std::unique_ptr<HrnSectionData> n) {
 if(!isRigidTransform(m,1e-9)||!p||!n||p.get()==n.get()) return std::unique_ptr<HrnPositionPointInput>();
 return std::unique_ptr<HrnPositionPointInput>(new HrnPositionPointInput(m,std::move(p),std::move(n)));
}
bool HrnPositionPointInput::append(ZeHrnSide s,HrnSegment& segment,std::unique_ptr<HrnSectionData> section) {
 if(!section) return false;
 for(const SideData& data:sides_) if(std::find(data.segments.begin(),data.segments.end(),&segment)!=data.segments.end()) return false;
 SideData& data=side(s); data.segments.push_back(&segment); data.sections.push_back(std::move(section)); return true;
}
size_t HrnPositionPointInput::sectionCount(ZeHrnSide s) const { return side(s).sections.size(); }
HrnSegment& HrnPositionPointInput::segmentAt(ZeHrnSide s,size_t i) const { return *side(s).segments.at(i); }
const HrnSectionData& HrnPositionPointInput::sectionAt(ZeHrnSide s,size_t i) const { return *side(s).sections.at(i); }
const HrnSectionData& HrnPositionPointInput::sideRootSection(ZeHrnSide s) const { return *side(s).root; }
bool HrnPositionPointInput::findSectionIndex(ZeHrnSide s,const HrnSegment& segment,size_t& index) const { const auto& entries=side(s).segments; auto it=std::find(entries.begin(),entries.end(),&segment); if(it==entries.end()) return false; index=static_cast<size_t>(it-entries.begin()); return true; }
const HrnSectionData* HrnPositionPointInput::getEffectiveRootSection(ZeHrnSide s) const {
 const SideData& d=side(s); size_t roots=0,root_index=0;
 for(size_t i=0;i<d.segments.size();++i) if(!d.segments[i]->parent()) { ++roots; root_index=i; }
 return roots==0?0:(roots==1?d.sections[root_index].get():d.root.get());
}
}
