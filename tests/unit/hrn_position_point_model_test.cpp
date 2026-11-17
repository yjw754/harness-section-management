#include "hrn/section/HrnPositionPoint.h"
#include "hrn/section/HrnPositionPointInput.h"
#include "hrn/section/HrnRootSectionDefaultProvider.h"
#include "TestSupport.h"
using namespace hrn;
static std::unique_ptr<HrnSectionData> section(double r=4) {
 HrnSectionOptions o; return HrnSectionData::create(HrnCircularSectionShape::create(r,1e-6),std::unique_ptr<HrnSectionLayout>(new HrnRuleBasedLayout()),o);
}
int main() {
 Matrix4D m; m.m[0][3]=7;
 CHECK(!HrnPositionPointInput::create(m,std::unique_ptr<HrnSectionData>(),section()));
 Matrix4D bad=m; bad.m[0][0]=2;
 CHECK(!HrnPositionPointInput::create(bad,section(),section()));
 auto input=HrnPositionPointInput::create(m,section(),section()); CHECK(input);
 CHECK(input->getEffectiveRootSection(ZeHrnSide::Positive)==0);
 HrnSegment a,b; CHECK(input->append(ZeHrnSide::Positive,a,section()));
 CHECK(!input->append(ZeHrnSide::Positive,a,section()));
 CHECK(input->append(ZeHrnSide::Negative,a,section()));
 CHECK(input->getEffectiveRootSection(ZeHrnSide::Positive)==&input->sectionAt(ZeHrnSide::Positive,0));
 CHECK(input->append(ZeHrnSide::Positive,b,section()));
 CHECK(input->getEffectiveRootSection(ZeHrnSide::Positive)==&input->sideRootSection(ZeHrnSide::Positive));
 size_t index=99; CHECK(input->findSectionIndex(ZeHrnSide::Positive,a,index)&&index==0);
 HrnRootSectionDefaults d; d.radius=8; d.solve_settings=HrnLayoutSolveSettings(3,55,1e-8); d.auto_resize_enabled=true;
 HrnRootSectionDefaultProvider provider(d);
 auto root=provider.create(ZeHrnSide::Positive); CHECK(root); CHECK(root->shape().dimensions()==HrnSectionDimensions::circle(8));
 CHECK(!provider.create(static_cast<ZeHrnSide>(99)));
 CHECK(root->layout().solveSettings()==d.solve_settings); CHECK(root->autoResizeEnabled());
 auto empty=HrnPositionPoint::createEmpty(m,provider); CHECK(empty); CHECK(empty->sectionCount(ZeHrnSide::Positive)==0);
 CHECK(empty->getEffectiveRootSection(ZeHrnSide::Positive)==0);
 auto clone=empty->clone(); CHECK(clone); CHECK(&clone->sideRootSection(ZeHrnSide::Positive)!=&empty->sideRootSection(ZeHrnSide::Positive));
 CHECK(clone->positionTransform()==m);
 HrnSegment external, child; CHECK(child.attachTo(external));
 auto roots=HrnPositionPointInput::create(m,section(),section()); CHECK(roots);
 CHECK(roots->append(ZeHrnSide::Positive,child,section()));
 CHECK(roots->getEffectiveRootSection(ZeHrnSide::Positive)==&roots->sectionAt(ZeHrnSide::Positive,0));
 CHECK(roots->append(ZeHrnSide::Negative,child,section()));
 CHECK(roots->getEffectiveRootSection(ZeHrnSide::Negative)==&roots->sectionAt(ZeHrnSide::Negative,0));
 CHECK(!roots->append(static_cast<ZeHrnSide>(99),external,section()));
 HrnSectionData* raw1=section().release();
 CHECK(!HrnPositionPointInput::create(m,std::unique_ptr<HrnSectionData>(raw1),std::unique_ptr<HrnSectionData>(raw1)));
 HrnSectionData* raw2=section().release();
 CHECK(!HrnPositionPoint::createEmpty(m,std::unique_ptr<HrnSectionData>(raw2),std::unique_ptr<HrnSectionData>(raw2)));
}
