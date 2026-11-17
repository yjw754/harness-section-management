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
 if(side!=ZeHrnSide::Positive&&side!=ZeHrnSide::Negative) { valid_=false; return *this; }
 const int i=side==ZeHrnSide::Positive?0:1;
 if(has_root_[i]) valid_=false;
 root_options_[i]=options; has_root_[i]=true; return *this;
}
HrnSectionCaseBuilder& HrnSectionCaseBuilder::circularSection(ZeHrnSide side,const std::string& label,const HrnCircularSectionCaseOptions& options) { if(side!=ZeHrnSide::Positive&&side!=ZeHrnSide::Negative) valid_=false; entries_.push_back({side,label,options}); return *this; }
HrnSectionCaseBuilder& HrnSectionCaseBuilder::relate(const std::string& p,const std::string& n,bool same) { relations_.push_back({p,n,same}); return *this; }
HrnSectionTestCase HrnSectionCaseBuilder::build() {
 HrnSectionTestCase result;
 if(!valid_||!has_transform_||!std::isfinite(modeling_tolerance_)||modeling_tolerance_<=0||!has_root_[0]||!has_root_[1]) { result.error="missing or invalid matrix, tolerance, root, or topology"; return result; }
 std::unique_ptr<HrnSectionData> positive_root=createCircular(root_options_[0],modeling_tolerance_);
 std::unique_ptr<HrnSectionData> negative_root=createCircular(root_options_[1],modeling_tolerance_);
 if(!positive_root||!negative_root) { result.error="invalid side Root Section"; return result; }
 std::set<HrnSegment*> membership[2];
 for(const Entry& e:entries_) {
  HrnSegment* p=graph_.find(e.label);
  const int side=e.side==ZeHrnSide::Positive?0:1;
  if(!p||!membership[side].insert(p).second||!createCircular(e.options,modeling_tolerance_)) { result.error="invalid, empty, or repeated Segment section"; return result; }
 }
 std::set<std::pair<HrnSegment*,HrnSegment*> > pairs;
 std::vector<HrnSegmentSideRelation> relations;
 for(const RelationEntry& e:relations_) {
  HrnSegment* p=graph_.find(e.positive); HrnSegment* n=graph_.find(e.negative);
  if(!p||!n||!membership[0].count(p)||!membership[1].count(n)||!pairs.insert(std::make_pair(p,n)).second) { result.error="invalid or repeated side relation"; return result; }
  relations.push_back({p,n,e.same_position});
 }
 result.input=HrnPositionPointInput::create(transform_,std::move(positive_root),std::move(negative_root));
 if(!result.input) { result.error="invalid Position Point input"; return result; }
 for(const Entry& e:entries_) if(!result.input->append(e.side,*graph_.find(e.label),createCircular(e.options,modeling_tolerance_))) { result.error="section append failed"; result.input.reset(); return result; }
 result.graph=std::move(graph_); result.relations=std::move(relations); result.modeling_tolerance=modeling_tolerance_; result.succeeded=true; return result;
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
