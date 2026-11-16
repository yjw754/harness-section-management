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
HrnSectionTestCase makeFiveLevelMappedCircularCase() {
 Matrix4D matrix; matrix.m[0][0]=0; matrix.m[0][1]=-1; matrix.m[1][0]=1; matrix.m[1][1]=0;
 matrix.m[0][3]=10; matrix.m[1][3]=20; matrix.m[2][3]=30;
 HrnCircularSectionCaseOptions root; root.radius=20; root.auto_resize_enabled=true;
 HrnCircularSectionCaseOptions member; member.radius=2; member.spacing=0.2; member.solve_settings=HrnLayoutSolveSettings(2,100,1e-8);
 HrnSectionCaseBuilder b;
 b.positionTransform(matrix).modelingTolerance(1e-6)
  .segment("P1").segment("P2").segment("P3").segment("P4").segment("P5").segment("P6")
  .segment("N1").segment("N2").segment("N3")
  .parent("P2","P1").parent("P3","P2").parent("P4","P3").parent("P5","P4")
  .sideRootCircular(ZeHrnSide::Positive,root).sideRootCircular(ZeHrnSide::Negative,root);
 for(const char* label:{"P1","P2","P3","P4","P5","P6"}) b.circularSection(ZeHrnSide::Positive,label,member);
 for(const char* label:{"N1","N2","N3"}) b.circularSection(ZeHrnSide::Negative,label,member);
 b.relate("P1","N1",true).relate("P1","N2",false).relate("P2","N2",false).relate("P3","N3",true);
 return b.build();
}
}
