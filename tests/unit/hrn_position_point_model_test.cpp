#include <gtest/gtest.h>

#include "hrn/section/HrnPositionPoint.h"
#include "hrn/section/HrnPositionPointInput.h"
#include "hrn/section/HrnRootSectionDefaultProvider.h"
using namespace hrn;
static std::unique_ptr<HrnSectionData> section(double r = 4) {
  HrnSectionOptions o;
  return HrnSectionData::create(HrnCircularSectionShape::create(r, 1e-6),
                                std::unique_ptr<HrnSectionLayout>(new HrnRuleBasedLayout()), o);
}
TEST(HrnPositionPointModel, CandidateRootsAndFormalEmpty) {
  Matrix4D m;
  m.m[0][3] = 7;
  ASSERT_TRUE(!HrnPositionPointInput::create(m, std::unique_ptr<HrnSectionData>(), section()));
  Matrix4D bad = m;
  bad.m[0][0] = 2;
  ASSERT_TRUE(!HrnPositionPointInput::create(bad, section(), section()));
  auto input = HrnPositionPointInput::create(m, section(), section());
  ASSERT_TRUE(input);
  ASSERT_TRUE(input->getEffectiveRootSection(ZeHrnSide::Positive) == 0);
  HrnSegment a, b;
  ASSERT_TRUE(input->append(ZeHrnSide::Positive, a, section()));
  ASSERT_TRUE(!input->append(ZeHrnSide::Positive, a, section()));
  ASSERT_TRUE(input->append(ZeHrnSide::Negative, a, section()));
  ASSERT_TRUE(input->getEffectiveRootSection(ZeHrnSide::Positive) ==
              &input->sectionAt(ZeHrnSide::Positive, 0));
  ASSERT_TRUE(input->append(ZeHrnSide::Positive, b, section()));
  ASSERT_TRUE(input->getEffectiveRootSection(ZeHrnSide::Positive) ==
              &input->sideRootSection(ZeHrnSide::Positive));
  size_t index = 99;
  ASSERT_TRUE(input->findSectionIndex(ZeHrnSide::Positive, a, index) && index == 0);
  HrnRootSectionDefaults d;
  d.radius = 8;
  d.solve_settings = HrnLayoutSolveSettings(3, 55, 1e-8);
  d.auto_resize_enabled = true;
  HrnRootSectionDefaultProvider provider(d);
  auto root = provider.create(ZeHrnSide::Positive);
  ASSERT_TRUE(root);
  ASSERT_TRUE(root->shape().dimensions() == HrnSectionDimensions::circle(8));
  ASSERT_TRUE(!provider.create(static_cast<ZeHrnSide>(99)));
  ASSERT_TRUE(root->layout().solveSettings() == d.solve_settings);
  ASSERT_TRUE(root->autoResizeEnabled());
  auto empty = HrnPositionPoint::createEmpty(m, provider);
  ASSERT_TRUE(empty);
  ASSERT_TRUE(empty->sectionCount(ZeHrnSide::Positive) == 0);
  ASSERT_TRUE(empty->getEffectiveRootSection(ZeHrnSide::Positive) == 0);
  auto clone = empty->clone();
  ASSERT_TRUE(clone);
  ASSERT_TRUE(&clone->sideRootSection(ZeHrnSide::Positive) !=
              &empty->sideRootSection(ZeHrnSide::Positive));
  ASSERT_TRUE(clone->positionTransform() == m);
  HrnSegment external, child;
  ASSERT_TRUE(child.attachTo(external));
  auto roots = HrnPositionPointInput::create(m, section(), section());
  ASSERT_TRUE(roots);
  ASSERT_TRUE(roots->append(ZeHrnSide::Positive, child, section()));
  ASSERT_TRUE(roots->getEffectiveRootSection(ZeHrnSide::Positive) ==
              &roots->sectionAt(ZeHrnSide::Positive, 0));
  ASSERT_TRUE(roots->append(ZeHrnSide::Negative, child, section()));
  ASSERT_TRUE(roots->getEffectiveRootSection(ZeHrnSide::Negative) ==
              &roots->sectionAt(ZeHrnSide::Negative, 0));
  ASSERT_TRUE(!roots->append(static_cast<ZeHrnSide>(99), external, section()));
  HrnSectionData* raw1 = section().release();
  ASSERT_TRUE(!HrnPositionPointInput::create(m, std::unique_ptr<HrnSectionData>(raw1),
                                             std::unique_ptr<HrnSectionData>(raw1)));
  HrnSectionData* raw2 = section().release();
  ASSERT_TRUE(!HrnPositionPoint::createEmpty(m, std::unique_ptr<HrnSectionData>(raw2),
                                             std::unique_ptr<HrnSectionData>(raw2)));
}

TEST(HrnPositionPointModel, RejectsAliasedOwnedObjectsAcrossSides) {
  Matrix4D matrix;
  HrnSegment a, b;
  {
    auto candidate = HrnPositionPointInput::create(matrix, section(), section());
    ASSERT_TRUE(candidate);
    HrnSectionData* shared = section(1).release();
    ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, a,
                                  std::unique_ptr<HrnSectionData>(shared)));
    const bool accepted = candidate->append(ZeHrnSide::Negative, b,
                                            std::unique_ptr<HrnSectionData>(shared));
    EXPECT_FALSE(accepted);
    if (accepted) candidate.release(); // Keep the failing pre-fix run from double-freeing.
  }
  {
    auto candidate = HrnPositionPointInput::create(matrix, section(), section());
    ASSERT_TRUE(candidate);
    HrnSectionShape* shared_shape = HrnCircularSectionShape::create(1, 1e-9).release();
    HrnSectionOptions options;
    auto first = HrnSectionData::create(std::unique_ptr<HrnSectionShape>(shared_shape),
        std::unique_ptr<HrnSectionLayout>(new HrnRuleBasedLayout()), options);
    auto second = HrnSectionData::create(std::unique_ptr<HrnSectionShape>(shared_shape),
        std::unique_ptr<HrnSectionLayout>(new HrnRuleBasedLayout()), options);
    ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, a, std::move(first)));
    const bool accepted = candidate->append(ZeHrnSide::Negative, b, std::move(second));
    EXPECT_FALSE(accepted);
    if (accepted) candidate.release();
  }
  {
    auto candidate = HrnPositionPointInput::create(matrix, section(), section());
    ASSERT_TRUE(candidate);
    HrnSectionLayout* shared_layout = new HrnRuleBasedLayout();
    HrnSectionOptions options;
    auto first = HrnSectionData::create(HrnCircularSectionShape::create(1, 1e-9),
        std::unique_ptr<HrnSectionLayout>(shared_layout), options);
    auto second = HrnSectionData::create(HrnCircularSectionShape::create(1, 1e-9),
        std::unique_ptr<HrnSectionLayout>(shared_layout), options);
    ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, a, std::move(first)));
    const bool accepted = candidate->append(ZeHrnSide::Negative, b, std::move(second));
    EXPECT_FALSE(accepted);
    if (accepted) candidate.release();
  }
  {
    HrnSectionShape* shared_shape = HrnCircularSectionShape::create(1, 1e-9).release();
    HrnSectionOptions options;
    auto positive = HrnSectionData::create(std::unique_ptr<HrnSectionShape>(shared_shape),
        std::unique_ptr<HrnSectionLayout>(new HrnRuleBasedLayout()), options);
    auto negative = HrnSectionData::create(std::unique_ptr<HrnSectionShape>(shared_shape),
        std::unique_ptr<HrnSectionLayout>(new HrnRuleBasedLayout()), options);
    auto candidate = HrnPositionPointInput::create(matrix, std::move(positive),
                                                    std::move(negative));
    EXPECT_FALSE(candidate);
    if (candidate) candidate.release();
  }
  {
    HrnSectionLayout* shared_layout = new HrnRuleBasedLayout();
    HrnSectionOptions options;
    auto positive = HrnSectionData::create(HrnCircularSectionShape::create(1, 1e-9),
        std::unique_ptr<HrnSectionLayout>(shared_layout), options);
    auto negative = HrnSectionData::create(HrnCircularSectionShape::create(1, 1e-9),
        std::unique_ptr<HrnSectionLayout>(shared_layout), options);
    auto point = HrnPositionPoint::createEmpty(matrix, std::move(positive),
                                               std::move(negative));
    EXPECT_FALSE(point);
    if (point) point.release();
  }
}
