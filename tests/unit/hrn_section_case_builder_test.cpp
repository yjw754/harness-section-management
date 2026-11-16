#include "HrnSectionCaseBuilder.h"
#include "TestSupport.h"
using namespace hrn;
static HrnCircularSectionCaseOptions circle(double r) { HrnCircularSectionCaseOptions o; o.radius=r; o.spacing=.2; return o; }
static HrnSectionCaseBuilder base() {
 HrnSectionCaseBuilder b; Matrix4D m; b.positionTransform(m).modelingTolerance(1e-6).segment("P1").segment("N1").segment("N2");
 b.sideRootCircular(ZeHrnSide::Positive,circle(10)).sideRootCircular(ZeHrnSide::Negative,circle(10));
 b.circularSection(ZeHrnSide::Positive,"P1",circle(3)).circularSection(ZeHrnSide::Negative,"N1",circle(2)).circularSection(ZeHrnSide::Negative,"N2",circle(1)); return b;
}
int main() {
 auto b=base(); b.relate("P1","N1",true).relate("P1","N2",false);
 auto c=b.build(); CHECK(c.succeeded); CHECK(c.relations.size()==2); CHECK(c.relations[0].same_position); CHECK(!c.relations[1].same_position);
 CHECK(c.input->sectionCount(ZeHrnSide::Positive)==1); CHECK(c.input->sectionCount(ZeHrnSide::Negative)==2);
 auto duplicate=base(); duplicate.relate("P1","N1",true).relate("P1","N1",true); CHECK(!duplicate.build().succeeded);
 auto conflict=base(); conflict.relate("P1","N1",true).relate("P1","N1",false); CHECK(!conflict.build().succeeded);
 auto wrong=base(); wrong.relate("N1","P1",true); CHECK(!wrong.build().succeeded);
 auto missing=base(); missing.relate("P1","missing",false); CHECK(!missing.build().succeeded);
 HrnSectionCaseBuilder noRoot; noRoot.positionTransform(Matrix4D()).modelingTolerance(1e-6).segment("P").sideRootCircular(ZeHrnSide::Positive,circle(10)).circularSection(ZeHrnSide::Positive,"P",circle(1)); CHECK(!noRoot.build().succeeded);
 HrnSectionCaseBuilder noMatrix; noMatrix.modelingTolerance(1e-6).sideRootCircular(ZeHrnSide::Positive,circle(10)).sideRootCircular(ZeHrnSide::Negative,circle(10)); CHECK(!noMatrix.build().succeeded);
 auto badTolerance=base(); badTolerance.modelingTolerance(0); CHECK(!badTolerance.build().succeeded);
 auto duplicateSegment=base(); duplicateSegment.segment("P1"); CHECK(!duplicateSegment.build().succeeded);
 auto crossSide=base(); crossSide.circularSection(ZeHrnSide::Negative,"P1",circle(1)); CHECK(!crossSide.build().succeeded);
 auto badSettings=base(); HrnCircularSectionCaseOptions options=circle(2); options.solve_settings=HrnLayoutSolveSettings(0,0,0); badSettings.sideRootCircular(ZeHrnSide::Positive,options); CHECK(!badSettings.build().succeeded);
}
