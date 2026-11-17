#include <gtest/gtest.h>

#include <set>

#include "HrnSectionCaseBuilder.h"
using namespace hrn;
TEST(Stage1Acceptance, FiveLevelMappedCircularCandidate) {
  HrnSectionTestCase c = makeFiveLevelMappedCircularCase();
  ASSERT_TRUE(c.succeeded);
  ASSERT_TRUE(c.input);
  ASSERT_TRUE(c.input->sectionCount(ZeHrnSide::Positive) == 6);
  ASSERT_TRUE(c.input->sectionCount(ZeHrnSide::Negative) == 3);
  ASSERT_TRUE(c.graph.depth("P5") == 5);
  ASSERT_TRUE(c.relations.size() == 4);
  size_t same = 0;
  for (const auto& relation : c.relations)
    if (relation.same_position) ++same;
  ASSERT_TRUE(same == 2);
  ASSERT_TRUE(c.relations[0].positive == c.relations[1].positive);
  ASSERT_TRUE(c.relations[1].negative == c.relations[2].negative);
  std::set<const void*> addresses;
  addresses.insert(&c.input->sideRootSection(ZeHrnSide::Positive));
  addresses.insert(&c.input->sideRootSection(ZeHrnSide::Negative));
  for (ZeHrnSide side : {ZeHrnSide::Positive, ZeHrnSide::Negative})
    for (size_t i = 0; i < c.input->sectionCount(side); ++i) {
      addresses.insert(&c.input->sectionAt(side, i));
      addresses.insert(&c.input->sectionAt(side, i).shape());
      addresses.insert(&c.input->sectionAt(side, i).layout());
    }
  ASSERT_TRUE(addresses.size() == 2 + 3 * (6 + 3));
  Matrix4D expected;
  expected.m[0][0] = 0;
  expected.m[0][1] = -1;
  expected.m[1][0] = 1;
  expected.m[1][1] = 0;
  expected.m[0][3] = 10;
  expected.m[1][3] = 20;
  expected.m[2][3] = 30;
  ASSERT_TRUE(c.input->positionTransform() == expected);
  ASSERT_TRUE(c.input->sideRootSection(ZeHrnSide::Positive).autoResizeEnabled());
  ASSERT_TRUE(c.input->sectionAt(ZeHrnSide::Positive, 0).layout().solveSettings() ==
              HrnLayoutSolveSettings(2, 100, 1e-8));
  ASSERT_TRUE(c.input->getEffectiveRootSection(ZeHrnSide::Positive) ==
              &c.input->sideRootSection(ZeHrnSide::Positive));
  ASSERT_TRUE(c.input->getEffectiveRootSection(ZeHrnSide::Negative) ==
              &c.input->sideRootSection(ZeHrnSide::Negative));
}
