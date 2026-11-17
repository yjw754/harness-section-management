#include <gtest/gtest.h>

#include <cmath>

#include "hrn/section/HrnPositionPointSolver.h"

using namespace hrn;
namespace {
std::unique_ptr<HrnSectionData> section(double radius, bool resize = false,
                                        double rotation = 0, double spacing = 0,
                                        HrnSectionSizeUpdateMode mode =
                                            HrnSectionSizeUpdateMode::KeepRatioConstant) {
  HrnSectionOptions options;
  options.auto_resize_enabled = resize;
  options.rotation = rotation;
  options.size_update_mode = mode;
  std::unique_ptr<HrnRuleBasedLayout> layout(new HrnRuleBasedLayout());
  layout->setSpacingTolerance(spacing);
  return HrnSectionData::create(HrnCircularSectionShape::create(radius, 1e-9),
                                std::move(layout), options);
}
std::unique_ptr<HrnPositionPointInput> input(const Matrix4D& matrix) {
  return HrnPositionPointInput::create(matrix, section(20, true), section(20, true));
}
}

TEST(HrnCircularHierarchySolver, ZeroAndSingleRootPreserveInactiveSideRoot) {
  Matrix4D matrix;
  matrix.m[0][3] = 7;
  matrix.m[1][3] = 8;
  matrix.m[2][3] = 9;
  auto empty = input(matrix);
  ASSERT_TRUE(empty);
  HrnPositionPointSolver solver;
  auto zero = solver.solveCircular(*empty, {});
  ASSERT_TRUE(zero.status.committed);
  EXPECT_TRUE(zero.side(ZeHrnSide::Positive).locations.empty());
  EXPECT_DOUBLE_EQ(zero.side(ZeHrnSide::Positive).root->shape().dimensions().primary, 20);
  HrnSegment only;
  ASSERT_TRUE(empty->append(ZeHrnSide::Positive, only, section(1, true)));
  auto single = solver.solveCircular(*empty, {});
  ASSERT_TRUE(single.status.committed);
  ASSERT_EQ(single.side(ZeHrnSide::Positive).locations.size(), 1u);
  EXPECT_DOUBLE_EQ(single.side(ZeHrnSide::Positive).locations[0].x, 7);
  EXPECT_DOUBLE_EQ(single.side(ZeHrnSide::Positive).locations[0].y, 8);
  EXPECT_DOUBLE_EQ(single.side(ZeHrnSide::Positive).locations[0].z, 9);
  EXPECT_DOUBLE_EQ(single.side(ZeHrnSide::Positive).sections[0]->shape().dimensions().primary, 1);
  EXPECT_DOUBLE_EQ(single.side(ZeHrnSide::Positive).root->shape().dimensions().primary, 20);
}

TEST(HrnCircularHierarchySolver, FiveLevelsAndRotatedParentUseSharedMatrixBasis) {
  Matrix4D matrix;
  matrix.m[0][0] = 0; matrix.m[0][1] = -1; matrix.m[1][0] = 1; matrix.m[1][1] = 0;
  matrix.m[0][3] = 7; matrix.m[1][3] = 8; matrix.m[2][3] = 9;
  auto candidate = input(matrix);
  HrnSegment nodes[5], sibling;
  for (int i = 1; i < 5; ++i) ASSERT_TRUE(nodes[i].attachTo(nodes[i - 1]));
  ASSERT_TRUE(sibling.attachTo(nodes[0]));
  for (int i = 0; i < 5; ++i)
    ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, nodes[i],
                                  section(i == 0 ? 50 : 10, false, 15)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, sibling, section(1)));
  HrnPositionPointSolver solver;
  auto solved = solver.solveCircular(*candidate, {});
  ASSERT_TRUE(solved.status.committed);
  const auto& positive = solved.side(ZeHrnSide::Positive);
  EXPECT_NEAR(positive.absolute_rotations[4], 75, 1e-9);
  EXPECT_NEAR(positive.centers2d[5].x, 10 * std::cos(15 * 3.14159265358979323846 / 180), 1e-9);
  EXPECT_NEAR(positive.centers2d[5].y, 10 * std::sin(15 * 3.14159265358979323846 / 180), 1e-9);
  EXPECT_NEAR(positive.locations[5].x, 7 - positive.centers2d[5].y, 1e-9);
  EXPECT_NEAR(positive.locations[5].y, 8 + positive.centers2d[5].x, 1e-9);
  EXPECT_DOUBLE_EQ(positive.locations[5].z, 9);
  EXPECT_DOUBLE_EQ(positive.locations[0].x, 7);
}

TEST(HrnCircularHierarchySolver, AutoRadiusAndFixedBoundary) {
  Matrix4D matrix;
  auto candidate = input(matrix);
  HrnSegment parent, first, second;
  ASSERT_TRUE(first.attachTo(parent));
  ASSERT_TRUE(second.attachTo(parent));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, parent, section(1, true)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, first, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, second, section(1)));
  HrnPositionPointSolver solver;
  auto solved = solver.solveCircular(*candidate, {});
  ASSERT_TRUE(solved.status.committed);
  EXPECT_NEAR(solved.side(ZeHrnSide::Positive).sections[0]->shape().dimensions().primary, 2, 1e-9);

  auto fixed = input(matrix);
  ASSERT_TRUE(fixed->append(ZeHrnSide::Positive, parent, section(1.5)));
  ASSERT_TRUE(fixed->append(ZeHrnSide::Positive, first, section(1)));
  ASSERT_TRUE(fixed->append(ZeHrnSide::Positive, second, section(1)));
  auto failed = solver.solveCircular(*fixed, {});
  EXPECT_FALSE(failed.status.committed);
  EXPECT_EQ(failed.status.primary_reason, HrnSectionFailureReason::FixedBoundaryInsufficient);
  ASSERT_FALSE(failed.status.diagnostics.empty());
  EXPECT_EQ(failed.status.diagnostics.front().target.segment, &parent);
}

TEST(HrnCircularHierarchySolver, MultipleRootsOnBothSidesAndAllCircularSizeModes) {
  Matrix4D matrix;
  matrix.m[0][0] = 0; matrix.m[0][1] = -1;
  matrix.m[1][0] = 1; matrix.m[1][1] = 0;
  matrix.m[0][3] = 7; matrix.m[1][3] = 8;
  HrnPositionPointSolver solver;
  for (const auto mode : {HrnSectionSizeUpdateMode::KeepRatioConstant,
                          HrnSectionSizeUpdateMode::KeepSmallestValue,
                          HrnSectionSizeUpdateMode::KeepGreatestValue}) {
    auto candidate = HrnPositionPointInput::create(matrix, section(10, true, 0, 0, mode),
                                                    section(10, true, 0, 0, mode));
    HrnSegment positive_a, positive_b, negative_a, negative_b;
    ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, positive_a, section(1)));
    ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, positive_b, section(1)));
    ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, negative_a, section(1)));
    ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, negative_b, section(1)));
    const auto solved = solver.solveCircular(*candidate, {});
    ASSERT_TRUE(solved.status.committed);
    for (const auto side : {ZeHrnSide::Positive, ZeHrnSide::Negative}) {
      const auto& data = solved.side(side);
      EXPECT_NEAR(data.root->shape().dimensions().primary, 2, 1e-9);
      EXPECT_NEAR(data.centers2d[0].x, -1, 1e-9);
      EXPECT_NEAR(data.centers2d[1].x, 1, 1e-9);
      EXPECT_NEAR(data.locations[0].x, 7, 1e-9);
      EXPECT_NEAR(data.locations[0].y, 7, 1e-9);
      EXPECT_NEAR(data.locations[1].x, 7, 1e-9);
      EXPECT_NEAR(data.locations[1].y, 9, 1e-9);
    }
  }
}
