#include <gtest/gtest.h>

#include <cmath>
#include <limits>

#include "hrn/section/HrnSectionGeometryService.h"

using namespace hrn;

TEST(HrnCircularGeometry, DistanceAndWitnesses) {
  HrnCircularGeometryService geometry;
  const HrnCircle2D a{{0, 0}, 2};
  const HrnCircle2D separated{{5, 0}, 1};
  const auto gap = geometry.signedDistance(a, separated, 1e-9);
  ASSERT_TRUE(gap.valid);
  EXPECT_DOUBLE_EQ(gap.signed_distance, 2);
  EXPECT_DOUBLE_EQ(gap.lhs_witness.x, 2);
  EXPECT_DOUBLE_EQ(gap.rhs_witness.x, 4);
  EXPECT_DOUBLE_EQ(gap.lhs_witness.y, 0);
  EXPECT_DOUBLE_EQ(gap.rhs_witness.y, 0);
  EXPECT_FALSE(geometry.intersects(a, separated, 1e-9));

  const auto touching = geometry.signedDistance(a, HrnCircle2D{{3, 0}, 1}, 1e-9);
  EXPECT_DOUBLE_EQ(touching.signed_distance, 0);
  EXPECT_TRUE(geometry.intersects(a, HrnCircle2D{{3, 0}, 1}, 1e-9));

  const auto overlap = geometry.signedDistance(a, HrnCircle2D{{3, 0}, 2}, 1e-9);
  EXPECT_DOUBLE_EQ(overlap.signed_distance, -1);
  EXPECT_TRUE(geometry.intersects(a, HrnCircle2D{{3, 0}, 2}, 1e-9));

  const auto concentric = geometry.signedDistance(a, HrnCircle2D{{0, 0}, 1}, 1e-9);
  EXPECT_DOUBLE_EQ(concentric.signed_distance, -3);
  EXPECT_DOUBLE_EQ(concentric.lhs_witness.x, 2);
  EXPECT_DOUBLE_EQ(concentric.rhs_witness.x, -1);
}

TEST(HrnCircularGeometry, ContainmentAndClearance) {
  HrnCircularGeometryService geometry;
  const HrnCircle2D parent{{0, 0}, 5};
  const auto inside = geometry.containsWithClearance(parent, HrnCircle2D{{2, 0}, 2}, 1, 1e-9);
  ASSERT_TRUE(inside.valid);
  EXPECT_TRUE(inside.contained);
  EXPECT_DOUBLE_EQ(inside.minimum_clearance, 1);
  EXPECT_DOUBLE_EQ(inside.parent_witness.x, 5);
  EXPECT_DOUBLE_EQ(inside.child_witness.x, 4);

  const auto outside = geometry.containsWithClearance(parent, HrnCircle2D{{4, 0}, 2}, 0, 1e-9);
  EXPECT_FALSE(outside.contained);
  EXPECT_DOUBLE_EQ(outside.minimum_clearance, -1);
  EXPECT_DOUBLE_EQ(outside.parent_witness.x, 5);
  EXPECT_DOUBLE_EQ(outside.child_witness.x, 6);

  const auto insufficient = geometry.containsWithClearance(parent, HrnCircle2D{{2, 0}, 2}, 2, 1e-9);
  EXPECT_FALSE(insufficient.contained);
  EXPECT_DOUBLE_EQ(insufficient.minimum_clearance, 1);

  const auto concentric = geometry.containsWithClearance(parent, HrnCircle2D{{0, 0}, 2}, 0, 1e-9);
  EXPECT_DOUBLE_EQ(concentric.minimum_clearance, 3);
  EXPECT_DOUBLE_EQ(concentric.parent_witness.x, 5);
  EXPECT_DOUBLE_EQ(concentric.child_witness.x, 2);
}

TEST(HrnCircularGeometry, RejectsInvalidGeometryAndTolerance) {
  HrnCircularGeometryService geometry;
  const HrnCircle2D valid{{0, 0}, 2};
  EXPECT_FALSE(geometry.signedDistance(valid, HrnCircle2D{{0, 0}, 0}, 1e-9).valid);
  EXPECT_FALSE(geometry.signedDistance(valid,
                  HrnCircle2D{{std::numeric_limits<double>::infinity(), 0}, 1}, 1e-9).valid);
  EXPECT_FALSE(geometry.containsWithClearance(valid, valid, -1, 1e-9).valid);
  EXPECT_FALSE(geometry.containsWithClearance(valid, valid, 0, -1).valid);
  EXPECT_FALSE(geometry.intersects(valid, HrnCircle2D{{0, 0}, -1}, 1e-9));
}

TEST(HrnCircularGeometry, MinimumEnclosingCircleRecentersNestedMembers) {
  HrnCircularGeometryService geometry;
  const auto pair = geometry.minimumEnclosingCircle(
      {HrnCircle2D{{0, 0}, 1}, HrnCircle2D{{2, 0}, 1}}, 1e-9);
  ASSERT_TRUE(pair.valid);
  EXPECT_NEAR(pair.circle.center.x, 1, 1e-9);
  EXPECT_NEAR(pair.circle.center.y, 0, 1e-9);
  EXPECT_NEAR(pair.circle.radius, 2, 1e-9);
  const auto triangle = geometry.minimumEnclosingCircle(
      {HrnCircle2D{{0, 0}, 1}, HrnCircle2D{{2, 0}, 1},
       HrnCircle2D{{1, std::sqrt(3.0)}, 1}}, 1e-9);
  ASSERT_TRUE(triangle.valid);
  EXPECT_NEAR(triangle.circle.center.x, 1, 1e-9);
  EXPECT_NEAR(triangle.circle.center.y, 1 / std::sqrt(3.0), 1e-9);
  EXPECT_NEAR(triangle.circle.radius, 1 + 2 / std::sqrt(3.0), 1e-9);
}
