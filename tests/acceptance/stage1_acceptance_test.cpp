#include "HrnSectionCaseBuilder.h"
#include "TestSupport.h"
#include <set>
using namespace hrn;
int main() {
 HrnSectionTestCase c=makeFiveLevelMappedCircularCase(); CHECK(c.succeeded); CHECK(c.input);
 CHECK(c.input->sectionCount(ZeHrnSide::Positive)==6); CHECK(c.input->sectionCount(ZeHrnSide::Negative)==3);
 CHECK(c.graph.depth("P5")==5);
 CHECK(c.relations.size()==4);
 size_t same=0; for(const auto& relation:c.relations) if(relation.same_position) ++same; CHECK(same==2);
 CHECK(c.relations[0].positive==c.relations[1].positive); CHECK(c.relations[1].negative==c.relations[2].negative);
 std::set<const void*> addresses; addresses.insert(&c.input->sideRootSection(ZeHrnSide::Positive)); addresses.insert(&c.input->sideRootSection(ZeHrnSide::Negative));
 for(ZeHrnSide side:{ZeHrnSide::Positive,ZeHrnSide::Negative}) for(size_t i=0;i<c.input->sectionCount(side);++i) { addresses.insert(&c.input->sectionAt(side,i)); addresses.insert(&c.input->sectionAt(side,i).shape()); addresses.insert(&c.input->sectionAt(side,i).layout()); }
 CHECK(addresses.size()==2+3*(6+3));
 Matrix4D expected; expected.m[0][0]=0; expected.m[0][1]=-1; expected.m[1][0]=1; expected.m[1][1]=0; expected.m[0][3]=10; expected.m[1][3]=20; expected.m[2][3]=30;
 CHECK(c.input->positionTransform()==expected);
 CHECK(c.input->sideRootSection(ZeHrnSide::Positive).autoResizeEnabled());
 CHECK(c.input->sectionAt(ZeHrnSide::Positive,0).layout().solveSettings()==HrnLayoutSolveSettings(2,100,1e-8));
 CHECK(c.input->getEffectiveRootSection(ZeHrnSide::Positive)==&c.input->sideRootSection(ZeHrnSide::Positive));
 CHECK(c.input->getEffectiveRootSection(ZeHrnSide::Negative)==&c.input->sideRootSection(ZeHrnSide::Negative));
}
