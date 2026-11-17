#include "hrn/section/HrnSectionData.h"
#include "TestSupport.h"
#include <limits>
using namespace hrn;
int main() {
 CHECK(!HrnCircularSectionShape::create(0,1e-6));
 CHECK(!HrnCircularSectionShape::create(2,-1));
 HrnRuleBasedLayout bad;
 CHECK(!bad.setSpacingTolerance(-.1));
 CHECK(!bad.setSolveSettings(HrnLayoutSolveSettings(0,0,0)));
 CHECK(bad.setSpacingTolerance(.2));
 CHECK(bad.setSolveSettings(HrnLayoutSolveSettings(1,100,1e-9)));
 HrnSectionOptions opts;
 std::unique_ptr<HrnSectionData> section=HrnSectionData::create(HrnCircularSectionShape::create(3,1e-6),bad.clone(),opts);
 CHECK(section); CHECK(section->rotation()==0); CHECK(!section->autoResizeEnabled());
 CHECK(section->sizeUpdateMode()==HrnSectionSizeUpdateMode::KeepRatioConstant);
 CHECK(section->layout().type()==ZeHrnSectionLayoutType::RuleBased);
 CHECK(section->setRotation(390)); CHECK(section->rotation()==30);
 CHECK(!section->setRotation(std::numeric_limits<double>::infinity()));
 std::unique_ptr<HrnSectionData> copy=section->clone(); CHECK(copy); CHECK(&copy->shape()!=&section->shape()); CHECK(&copy->layout()!=&section->layout());
 CHECK(copy->shape().dimensions()==HrnSectionDimensions::circle(3));
 HrnSectionOptions invalidOptions; invalidOptions.size_update_mode=static_cast<HrnSectionSizeUpdateMode>(99);
 CHECK(!HrnSectionData::create(HrnCircularSectionShape::create(2,1e-6),bad.clone(),invalidOptions));
}
