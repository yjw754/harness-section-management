#include <gtest/gtest.h>

#include <limits>

#include "hrn/section/HrnSectionData.h"
using namespace hrn;
TEST(HrnSectionData, CircularOptionsAndClone) {
  ASSERT_TRUE(!HrnCircularSectionShape::create(0, 1e-6));
  ASSERT_TRUE(!HrnCircularSectionShape::create(2, -1));
  HrnRuleBasedLayout bad;
  ASSERT_TRUE(!bad.setSpacingTolerance(-.1));
  ASSERT_TRUE(!bad.setSolveSettings(HrnLayoutSolveSettings(0, 0, 0)));
  ASSERT_TRUE(bad.setSpacingTolerance(.2));
  ASSERT_TRUE(bad.setSolveSettings(HrnLayoutSolveSettings(1, 100, 1e-9)));
  HrnSectionOptions opts;
  std::unique_ptr<HrnSectionData> section =
      HrnSectionData::create(HrnCircularSectionShape::create(3, 1e-6), bad.clone(), opts);
  ASSERT_TRUE(section);
  ASSERT_TRUE(section->rotation() == 0);
  ASSERT_TRUE(!section->autoResizeEnabled());
  ASSERT_TRUE(section->sizeUpdateMode() == HrnSectionSizeUpdateMode::KeepRatioConstant);
  ASSERT_TRUE(section->layout().type() == ZeHrnSectionLayoutType::RuleBased);
  ASSERT_TRUE(section->setRotation(390));
  ASSERT_TRUE(section->rotation() == 30);
  ASSERT_TRUE(!section->setRotation(std::numeric_limits<double>::infinity()));
  std::unique_ptr<HrnSectionData> copy = section->clone();
  ASSERT_TRUE(copy);
  ASSERT_TRUE(&copy->shape() != &section->shape());
  ASSERT_TRUE(&copy->layout() != &section->layout());
  ASSERT_TRUE(copy->shape().dimensions() == HrnSectionDimensions::circle(3));
  HrnSectionOptions invalidOptions;
  invalidOptions.size_update_mode = static_cast<HrnSectionSizeUpdateMode>(99);
  ASSERT_TRUE(!HrnSectionData::create(HrnCircularSectionShape::create(2, 1e-6), bad.clone(),
                                      invalidOptions));
}
