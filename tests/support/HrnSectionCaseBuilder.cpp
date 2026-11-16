#include "HrnSectionCaseBuilder.h"
#include <cmath>
#include <map>
#include <set>
namespace hrn {
std::unique_ptr<HrnSectionData> HrnSectionCaseBuilder::createCircular(const HrnCircularSectionCaseOptions& options,double tolerance) {
 std::unique_ptr<HrnRuleBasedLayout> layout(new HrnRuleBasedLayout());
 if(!layout->setSpacingTolerance(options.spacing)||!layout->setSolveSettings(options.solve_settings)) return std::unique_ptr<HrnSectionData>();
 HrnSectionOptions o; o.rotation=options.rotation; o.auto_resize_enabled=options.auto_resize_enabled; o.size_update_mode=options.size_update_mode;
 return HrnSectionData::create(HrnCircularSectionShape::create(options.radius,tolerance),std::move(layout),o);
}
HrnSectionCaseBuilder& HrnSectionCaseBuilder::positionTransform(const Matrix4D& m) { transform_=m; has_transform_=true; if(!isRigidTransform(m,1e-9)) valid_=false; return *this; }
HrnSectionCaseBuilder& HrnSectionCaseBuilder::modelingTolerance(double t) { modeling_tolerance_=t; if(!std::isfinite(t)||t<=0) valid_=false; return *this; }
HrnSectionCaseBuilder& HrnSectionCaseBuilder::segment(const std::string& label) { HrnSegment* p=0; if(!graph_.tryAdd(label,p)) valid_=false; return *this; }
HrnSectionCaseBuilder& HrnSectionCaseBuilder::parent(const std::string& child,const std::string& parent_label) { HrnSegment* c=graph_.find(child); HrnSegment* p=graph_.find(parent_label); if(!c||!p||!graph_.setParent(*c,*p)) valid_=false; return *this; }
HrnSectionCaseBuilder& HrnSectionCaseBuilder::sideRootCircular(ZeHrnSide side,const HrnCircularSectionCaseOptions& options) {
 const int i=side==ZeHrnSide::Positive?0:1;
 if(roots_[i]) valid_=false;
 roots_[i]=createCircular(options,modeling_tolerance_);
 if(!roots_[i]) valid_=false; return *this;
}
HrnSectionCaseBuilder& HrnSectionCaseBuilder::circularSection(ZeHrnSide side,const std::string& label,const HrnCircularSectionCaseOptions& options) { entries_.push_back({side,label,options}); return *this; }
HrnSectionCaseBuilder& HrnSectionCaseBuilder::relate(const std::string& p,const std::string& n,bool same) { relations_.push_back({p,n,same}); return *this; }
HrnSectionTestCase HrnSectionCaseBuilder::build() {
 HrnSectionTestCase result;
 if(!valid_||!has_transform_||!std::isfinite(modeling_tolerance_)||modeling_tolerance_<=0||!roots_[0]||!roots_[1]) { result.error="missing or invalid matrix, tolerance, root, or topology"; return result; }
 std::map<HrnSegment*,ZeHrnSide> membership;
 for(const Entry& e:entries_) {
  HrnSegment* p=graph_.find(e.label);
  if(!p||membership.count(p)||!createCircular(e.options,modeling_tolerance_)) { result.error="invalid, empty, or repeated Segment section"; return result; }
  membership[p]=e.side;
 }
 for(const auto& member:membership) if(member.first->parent()) {
  auto parent=membership.find(member.first->parent());
  if(parent==membership.end()||parent->second!=member.second) { result.error="parent missing from same side"; return result; }
 }
 std::set<std::pair<HrnSegment*,HrnSegment*> > pairs;
 for(const RelationEntry& e:relations_) {
  HrnSegment* p=graph_.find(e.positive); HrnSegment* n=graph_.find(e.negative);
  if(!p||!n||!membership.count(p)||!membership.count(n)||membership[p]!=ZeHrnSide::Positive||membership[n]!=ZeHrnSide::Negative||!pairs.insert(std::make_pair(p,n)).second) { result.error="invalid or repeated side relation"; return result; }
  result.relations.push_back({p,n,e.same_position});
 }
 result.input=HrnPositionPointInput::create(transform_,std::move(roots_[0]),std::move(roots_[1]));
 if(!result.input) { result.error="invalid Position Point input"; return result; }
 for(const Entry& e:entries_) if(!result.input->append(e.side,*graph_.find(e.label),createCircular(e.options,modeling_tolerance_))) { result.error="section append failed"; result.input.reset(); return result; }
 result.graph=std::move(graph_); result.modeling_tolerance=modeling_tolerance_; result.succeeded=true; return result;
}
}
