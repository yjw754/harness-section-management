#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>
#include <vector>

#include "hrn/section/HrnPositionPoint.h"
#include "hrn/section/HrnPositionPointSolver.h"

using namespace hrn;
namespace {
std::unique_ptr<HrnSectionData> section(double radius, bool resize = false, double rotation = 0) {
  HrnSectionOptions options;
  options.auto_resize_enabled = resize;
  options.rotation = rotation;
  return HrnSectionData::create(HrnCircularSectionShape::create(radius, 1e-9),
      std::unique_ptr<HrnSectionLayout>(new HrnRuleBasedLayout()), options);
}
std::unique_ptr<HrnPositionPointInput> input(double root_radius = 10) {
  return HrnPositionPointInput::create(Matrix4D(), section(root_radius), section(root_radius));
}
}

TEST(HrnCircularTransaction, OrdinaryAndExplicitRelationsDiffer) {
  HrnSegment p, q, n;
  auto ordinary = input();
  ASSERT_TRUE(ordinary->append(ZeHrnSide::Positive, p, section(1)));
  ASSERT_TRUE(ordinary->append(ZeHrnSide::Positive, q, section(1)));
  ASSERT_TRUE(ordinary->append(ZeHrnSide::Negative, n, section(1)));
  HrnPositionPointSolver solver;
  HrnSectionTransactionResult status;
  auto point = HrnPositionPoint::createSolved(*ordinary, {{&p, &n, false}}, solver, status);
  ASSERT_TRUE(point);
  ASSERT_TRUE(status.committed);
  size_t pi = 0, ni = 0;
  ASSERT_TRUE(point->findSectionIndex(ZeHrnSide::Positive, p, pi));
  ASSERT_TRUE(point->findSectionIndex(ZeHrnSide::Negative, n, ni));
  EXPECT_NE(point->locationAt(ZeHrnSide::Positive, pi).x,
            point->locationAt(ZeHrnSide::Negative, ni).x);

  auto coupled = HrnPositionPoint::createSolved(*ordinary, {{&p, &n, true}}, solver, status);
  ASSERT_TRUE(coupled);
  ASSERT_TRUE(status.committed);
  ASSERT_TRUE(coupled->findSectionIndex(ZeHrnSide::Positive, p, pi));
  ASSERT_TRUE(coupled->findSectionIndex(ZeHrnSide::Negative, n, ni));
  EXPECT_DOUBLE_EQ(coupled->locationAt(ZeHrnSide::Positive, pi).x,
                   coupled->locationAt(ZeHrnSide::Negative, ni).x);
}

TEST(HrnCircularTransaction, SingletonFollowsAnyMultiRootEndpointWithoutRepacking) {
  HrnPositionPointSolver solver;
  for (auto multi : {ZeHrnSide::Positive, ZeHrnSide::Negative}) {
    const bool multi_is_positive = multi == ZeHrnSide::Positive;
    const auto single = multi_is_positive ? ZeHrnSide::Negative : ZeHrnSide::Positive;
    Matrix4D transform;
    transform.m[0][0] = 0; transform.m[0][1] = -1; transform.m[0][3] = 7;
    transform.m[1][0] = 1; transform.m[1][1] = 0; transform.m[1][3] = 8;
    transform.m[2][3] = 9;
    auto candidate = HrnPositionPointInput::create(transform, section(2.16), section(2.16));
    ASSERT_TRUE(candidate);
    HrnSegment nodes[3], follower;
    for (auto& node : nodes) ASSERT_TRUE(candidate->append(multi, node, section(1)));
    ASSERT_TRUE(candidate->append(single, follower, section(1)));
    HrnSectionTransactionResult status;
    const auto baseline = HrnPositionPoint::createSolved(*candidate, {}, solver, status);
    ASSERT_TRUE(baseline);
    EXPECT_DOUBLE_EQ(baseline->locationAt(single, 0).x, 7);
    EXPECT_DOUBLE_EQ(baseline->locationAt(single, 0).y, 8);
    for (size_t endpoint = 0; endpoint < 3; ++endpoint) {
      SCOPED_TRACE(::testing::Message() << "positive multi: " << multi_is_positive
                                       << ", endpoint: " << endpoint);
      const auto link = [&](size_t index, bool same_position) -> HrnSegmentSideRelation {
        return multi_is_positive ? HrnSegmentSideRelation{&nodes[index], &follower, same_position}
                                 : HrnSegmentSideRelation{&follower, &nodes[index], same_position};
      };
      const auto point = HrnPositionPoint::createSolved(
          *candidate, {link(endpoint, true), link((endpoint + 1) % 3, false)}, solver, status);
      ASSERT_TRUE(point);
      EXPECT_DOUBLE_EQ(point->sideRootSection(single).shape().dimensions().primary, 2.16);
      EXPECT_DOUBLE_EQ(point->sideRootSection(multi).shape().dimensions().primary, 2.16);
      for (size_t i = 0; i < 3; ++i) {
        const auto before = baseline->locationAt(multi, i);
        const auto after = point->locationAt(multi, i);
        EXPECT_NEAR(before.x, after.x, 1e-9);
        EXPECT_NEAR(before.y, after.y, 1e-9);
        EXPECT_NEAR(before.z, after.z, 1e-9);
      }
      const auto matched = point->locationAt(multi, endpoint);
      const auto followed = point->locationAt(single, 0);
      EXPECT_NEAR(followed.x, matched.x, 1e-9);
      EXPECT_NEAR(followed.y, matched.y, 1e-9);
      EXPECT_NEAR(followed.z, matched.z, 1e-9);
      EXPECT_GT(std::hypot(followed.x - 7, followed.y - 8), 1);
    }
  }
}

TEST(HrnCircularTransaction, SingletonFollowsNestedEndpointWithoutMovingItsAncestors) {
  const auto multi = ZeHrnSide::Positive;
  const auto single = ZeHrnSide::Negative;
  HrnSegment parent, other_root, peer, sibling, follower;
  ASSERT_TRUE(peer.attachTo(parent));
  ASSERT_TRUE(sibling.attachTo(parent));
  auto candidate = input(5);
  ASSERT_TRUE(candidate->append(multi, parent, section(2)));
  ASSERT_TRUE(candidate->append(multi, other_root, section(2)));
  ASSERT_TRUE(candidate->append(multi, peer, section(0.5)));
  ASSERT_TRUE(candidate->append(multi, sibling, section(0.5)));
  ASSERT_TRUE(candidate->append(single, follower, section(1)));

  HrnPositionPointSolver solver;
  HrnSectionTransactionResult status;
  const auto baseline = HrnPositionPoint::createSolved(*candidate, {}, solver, status);
  ASSERT_TRUE(baseline);
  const auto coupled = HrnPositionPoint::createSolved(
      *candidate, {{&peer, &follower, true}}, solver, status);
  ASSERT_TRUE(coupled);
  for (const auto* node : {&parent, &other_root, &peer, &sibling}) {
    size_t before_index = 0, after_index = 0;
    ASSERT_TRUE(baseline->findSectionIndex(multi, *node, before_index));
    ASSERT_TRUE(coupled->findSectionIndex(multi, *node, after_index));
    const auto before = baseline->locationAt(multi, before_index);
    const auto after = coupled->locationAt(multi, after_index);
    EXPECT_NEAR(before.x, after.x, 1e-9);
    EXPECT_NEAR(before.y, after.y, 1e-9);
  }
  size_t peer_index = 0, parent_index = 0;
  ASSERT_TRUE(coupled->findSectionIndex(multi, peer, peer_index));
  ASSERT_TRUE(coupled->findSectionIndex(multi, parent, parent_index));
  const auto matched = coupled->locationAt(multi, peer_index);
  const auto ancestor = coupled->locationAt(multi, parent_index);
  const auto followed = coupled->locationAt(single, 0);
  EXPECT_GT(std::hypot(matched.x, matched.y), 1);
  EXPECT_GT(std::hypot(matched.x - ancestor.x, matched.y - ancestor.y), 0.1);
  EXPECT_NEAR(followed.x, matched.x, 1e-9);
  EXPECT_NEAR(followed.y, matched.y, 1e-9);
}

TEST(HrnCircularTransaction, SingletonExceptionDoesNotUnpinNestedSingleRoot) {
  HrnSegment positive_a, positive_b, negative_root, negative_child;
  ASSERT_TRUE(negative_child.attachTo(negative_root));
  auto candidate = input();
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, positive_a, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, positive_b, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, negative_root, section(2)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, negative_child, section(1)));
  HrnPositionPointSolver solver;
  const auto solved = solver.solveCircular(*candidate, {{&positive_b, &negative_root, true}});
  ASSERT_TRUE(solved.status.committed);
  EXPECT_DOUBLE_EQ(solved.side(ZeHrnSide::Negative).centers2d[0].x, 0);
  EXPECT_DOUBLE_EQ(solved.side(ZeHrnSide::Negative).centers2d[0].y, 0);
  EXPECT_DOUBLE_EQ(solved.side(ZeHrnSide::Positive).centers2d[1].x, 0);
  EXPECT_DOUBLE_EQ(solved.side(ZeHrnSide::Positive).centers2d[1].y, 0);
}

TEST(HrnCircularTransaction, FailedEditAndInvalidMatrixRollBack) {
  HrnSegment p, q;
  auto candidate = input(2.5);
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, p, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, q, section(1)));
  HrnPositionPointSolver solver;
  HrnSectionTransactionResult status;
  auto point = HrnPositionPoint::createSolved(*candidate, {}, solver, status);
  ASSERT_TRUE(point);
  auto before = point->clone();
  auto failed = point->replaceSectionData(ZeHrnSide::Positive, p, section(2), solver);
  EXPECT_FALSE(failed.committed);
  EXPECT_EQ(failed.primary_reason, HrnSectionFailureReason::FixedBoundaryInsufficient);
  EXPECT_EQ(point->sectionAt(ZeHrnSide::Positive, 0).shape().dimensions(),
            before->sectionAt(ZeHrnSide::Positive, 0).shape().dimensions());
  EXPECT_DOUBLE_EQ(point->locationAt(ZeHrnSide::Positive, 0).x,
                   before->locationAt(ZeHrnSide::Positive, 0).x);
  Matrix4D mirror;
  mirror.m[0][0] = -1;
  auto invalid = point->replacePositionTransform(mirror, solver);
  EXPECT_FALSE(invalid.committed);
  EXPECT_EQ(invalid.primary_reason, HrnSectionFailureReason::InvalidInput);
  EXPECT_TRUE(point->positionTransform() == before->positionTransform());
}

TEST(HrnCircularTransaction, StableIdentityEditsKeepTriplesAligned) {
  HrnSegment a, b, c;
  auto candidate = input();
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, a, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, b, section(1)));
  HrnPositionPointSolver solver;
  HrnSectionTransactionResult status;
  auto point = HrnPositionPoint::createSolved(*candidate, {}, solver, status);
  ASSERT_TRUE(point);
  ASSERT_TRUE(point->appendSectionItem(ZeHrnSide::Positive, c, section(1), solver).committed);
  ASSERT_TRUE(point->reorderSectionItems(ZeHrnSide::Positive, {&c, &a, &b}, solver).committed);
  size_t resolved = 99;
  ASSERT_TRUE(point->findSectionIndex(ZeHrnSide::Positive, a, resolved));
  EXPECT_EQ(resolved, 1u);
  EXPECT_EQ(&point->segmentAt(ZeHrnSide::Positive, resolved), &a);
  EXPECT_DOUBLE_EQ(point->sectionAt(ZeHrnSide::Positive, resolved).shape().dimensions().primary, 1);
  ASSERT_TRUE(point->moveSectionItem(ZeHrnSide::Positive, ZeHrnSide::Negative, c, solver).committed);
  EXPECT_FALSE(point->findSectionIndex(ZeHrnSide::Positive, c, resolved));
  EXPECT_TRUE(point->findSectionIndex(ZeHrnSide::Negative, c, resolved));
  ASSERT_TRUE(point->removeSectionItem(ZeHrnSide::Positive, b, solver).committed);
  EXPECT_EQ(point->sectionCount(ZeHrnSide::Positive), 1u);
  ASSERT_TRUE(point->replaceSideRootSectionData(ZeHrnSide::Negative, section(20), solver).committed);
  EXPECT_DOUBLE_EQ(point->sideRootSection(ZeHrnSide::Negative).shape().dimensions().primary, 20);
}

TEST(HrnCircularTransaction, DeepCrossSideClassSharesGlobalCenter) {
  HrnSegment positive_root, positive_a, positive_b;
  HrnSegment negative_root, negative_a, negative_b;
  ASSERT_TRUE(positive_a.attachTo(positive_root));
  ASSERT_TRUE(positive_b.attachTo(positive_root));
  ASSERT_TRUE(negative_a.attachTo(negative_root));
  ASSERT_TRUE(negative_b.attachTo(negative_root));
  auto candidate = input(20);
  HrnSectionOptions rotated;
  rotated.rotation = 90;
  auto positive_parent = HrnSectionData::create(
      HrnCircularSectionShape::create(10, 1e-9),
      std::unique_ptr<HrnSectionLayout>(new HrnRuleBasedLayout()), rotated);
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, positive_root, std::move(positive_parent)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, positive_a, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, positive_b, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, negative_root, section(10)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, negative_a, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, negative_b, section(1)));
  HrnPositionPointSolver solver;
  HrnSectionTransactionResult status;
  auto point = HrnPositionPoint::createSolved(*candidate,
      {{&positive_b, &negative_b, true}}, solver, status);
  ASSERT_TRUE(point);
  ASSERT_TRUE(status.committed);
  const auto positive = point->locationAt(ZeHrnSide::Positive, 2);
  const auto negative = point->locationAt(ZeHrnSide::Negative, 2);
  EXPECT_NEAR(positive.x, negative.x, 1e-9);
  EXPECT_NEAR(positive.y, negative.y, 1e-9);
  EXPECT_NEAR(positive.z, negative.z, 1e-9);
}

TEST(HrnCircularTransaction, TwoNestedSiblingPairsShareCentersAcrossSides) {
  HrnSegment p0, p1, p2, n0, n1, n2;
  ASSERT_TRUE(p1.attachTo(p0));
  ASSERT_TRUE(p2.attachTo(p0));
  ASSERT_TRUE(n1.attachTo(n0));
  ASSERT_TRUE(n2.attachTo(n0));
  auto candidate = input(20);
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, p0, section(6)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, p1, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, p2, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, n0, section(6)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, n1, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, n2, section(1)));

  HrnPositionPointSolver solver;
  auto solved = solver.solveCircular(*candidate,
      {{&p1, &n1, true}, {&p2, &n2, true}});
  ASSERT_TRUE(solved.status.committed);
  const auto& positive = solved.side(ZeHrnSide::Positive);
  const auto& negative = solved.side(ZeHrnSide::Negative);
  ASSERT_EQ(positive.centers2d.size(), 3u);
  ASSERT_EQ(negative.centers2d.size(), 3u);
  for (size_t i : {1u, 2u}) {
    EXPECT_NEAR(positive.centers2d[i].x, negative.centers2d[i].x, 1e-9);
    EXPECT_NEAR(positive.centers2d[i].y, negative.centers2d[i].y, 1e-9);
    EXPECT_LE(std::hypot(positive.centers2d[i].x - positive.centers2d[0].x,
                         positive.centers2d[i].y - positive.centers2d[0].y) + 1, 6 + 1e-9);
    EXPECT_LE(std::hypot(negative.centers2d[i].x - negative.centers2d[0].x,
                         negative.centers2d[i].y - negative.centers2d[0].y) + 1, 6 + 1e-9);
  }
  EXPECT_GE(std::hypot(positive.centers2d[1].x - positive.centers2d[2].x,
                       positive.centers2d[1].y - positive.centers2d[2].y), 2 - 1e-9);
}

TEST(HrnCircularTransaction, OneNestedSiblingPairSharesCenterAcrossSides) {
  HrnSegment p0, p1, p2, n0, n1, n2;
  ASSERT_TRUE(p1.attachTo(p0));
  ASSERT_TRUE(p2.attachTo(p0));
  ASSERT_TRUE(n1.attachTo(n0));
  ASSERT_TRUE(n2.attachTo(n0));
  auto candidate = input(20);
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, p0, section(6)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, p1, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, p2, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, n0, section(6)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, n1, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, n2, section(1)));

  HrnPositionPointSolver solver;
  auto solved = solver.solveCircular(*candidate, {{&p1, &n1, true}});
  ASSERT_TRUE(solved.status.committed);
  const auto& positive = solved.side(ZeHrnSide::Positive);
  const auto& negative = solved.side(ZeHrnSide::Negative);
  ASSERT_EQ(positive.centers2d.size(), 3u);
  ASSERT_EQ(negative.centers2d.size(), 3u);
  EXPECT_NEAR(positive.centers2d[1].x, negative.centers2d[1].x, 1e-9);
  EXPECT_NEAR(positive.centers2d[1].y, negative.centers2d[1].y, 1e-9);
  for (const auto* side : {&positive, &negative}) {
    for (size_t i : {1u, 2u})
      EXPECT_LE(std::hypot(side->centers2d[i].x - side->centers2d[0].x,
                           side->centers2d[i].y - side->centers2d[0].y) + 1, 6 + 1e-9);
    EXPECT_GE(std::hypot(side->centers2d[1].x - side->centers2d[2].x,
                         side->centers2d[1].y - side->centers2d[2].y), 2 - 1e-9);
  }
}

TEST(HrnCircularTransaction, ThreeMappedPairsAcrossTwoNestedParents) {
  HrnSegment p0, p1, p2, p3, p4, n0, n1, n2, n3, n4;
  ASSERT_TRUE(p0.attachTo(p3));
  ASSERT_TRUE(p1.attachTo(p0));
  ASSERT_TRUE(p2.attachTo(p0));
  ASSERT_TRUE(p4.attachTo(p3));
  ASSERT_TRUE(n0.attachTo(n3));
  ASSERT_TRUE(n1.attachTo(n3));
  ASSERT_TRUE(n2.attachTo(n0));
  ASSERT_TRUE(n4.attachTo(n0));
  auto candidate = input(20);
  for (const auto side : {ZeHrnSide::Positive, ZeHrnSide::Negative}) {
    HrnSegment& outer = side == ZeHrnSide::Positive ? p3 : n3;
    HrnSegment& middle = side == ZeHrnSide::Positive ? p0 : n0;
    HrnSegment& first = side == ZeHrnSide::Positive ? p1 : n1;
    HrnSegment& second = side == ZeHrnSide::Positive ? p2 : n2;
    HrnSegment& outer_child = side == ZeHrnSide::Positive ? p4 : n4;
    if (side == ZeHrnSide::Negative) {
      HrnSectionOptions rotated;
      rotated.rotation = 180;
      ASSERT_TRUE(candidate->append(side, outer, HrnSectionData::create(
          HrnCircularSectionShape::create(12, 1e-9),
          std::unique_ptr<HrnSectionLayout>(new HrnRuleBasedLayout()), rotated)));
    } else {
      ASSERT_TRUE(candidate->append(side, outer, section(12)));
    }
    if (side == ZeHrnSide::Negative) {
      HrnSectionOptions rotated;
      rotated.rotation = 180;
      ASSERT_TRUE(candidate->append(side, middle, HrnSectionData::create(
          HrnCircularSectionShape::create(2, 1e-9),
          std::unique_ptr<HrnSectionLayout>(new HrnRuleBasedLayout()), rotated)));
    } else {
      ASSERT_TRUE(candidate->append(side, middle, section(2)));
    }
    ASSERT_TRUE(candidate->append(side, first, section(1)));
    ASSERT_TRUE(candidate->append(side, second, section(1)));
    ASSERT_TRUE(candidate->append(side, outer_child, section(1)));
  }
  HrnPositionPointSolver solver;
  auto solved = solver.solveCircular(*candidate,
      {{&p1, &n1, true}, {&p2, &n2, true}, {&p4, &n4, true}});
  ASSERT_TRUE(solved.status.committed);
  const auto& positive = solved.side(ZeHrnSide::Positive);
  const auto& negative = solved.side(ZeHrnSide::Negative);
  ASSERT_EQ(positive.centers2d.size(), 5u);
  ASSERT_EQ(negative.centers2d.size(), 5u);
  for (size_t i : {2u, 3u, 4u}) {
    EXPECT_NEAR(positive.centers2d[i].x, negative.centers2d[i].x, 1e-9);
    EXPECT_NEAR(positive.centers2d[i].y, negative.centers2d[i].y, 1e-9);
  }
  for (const auto* side : {&positive, &negative}) {
    const std::vector<size_t> inner_children =
        side == &positive ? std::vector<size_t>{2, 3} : std::vector<size_t>{3, 4};
    const size_t outer_child = side == &positive ? 4 : 2;
    for (size_t i : inner_children)
      EXPECT_LE(std::hypot(side->centers2d[i].x - side->centers2d[1].x,
                           side->centers2d[i].y - side->centers2d[1].y) + 1, 2 + 1e-9);
    EXPECT_LE(std::hypot(side->centers2d[1].x - side->centers2d[0].x,
                         side->centers2d[1].y - side->centers2d[0].y) + 2, 12 + 1e-9);
    EXPECT_LE(std::hypot(side->centers2d[outer_child].x - side->centers2d[0].x,
                         side->centers2d[outer_child].y - side->centers2d[0].y) + 1,
              12 + 1e-9);
    EXPECT_GE(std::hypot(side->centers2d[1].x - side->centers2d[outer_child].x,
                         side->centers2d[1].y - side->centers2d[outer_child].y), 3 - 1e-9);
  }
}

TEST(HrnCircularTransaction, CoupledParentsCanMoveWithoutDraggingMappedChildren) {
  for (bool swap_sides : {false, true}) {
    SCOPED_TRACE(swap_sides);
    const auto side_a = swap_sides ? ZeHrnSide::Negative : ZeHrnSide::Positive;
    const auto side_b = swap_sides ? ZeHrnSide::Positive : ZeHrnSide::Negative;
    HrnSegment a[4], b[4];
    ASSERT_TRUE(a[0].attachTo(a[3]));
    ASSERT_TRUE(a[1].attachTo(a[3]));
    ASSERT_TRUE(b[1].attachTo(b[3]));
    ASSERT_TRUE(b[2].attachTo(b[3]));
    auto candidate = input(20);
    std::vector<HrnSegmentSideRelation> relations;
    for (size_t i = 0; i < 4; ++i) {
      ASSERT_TRUE(candidate->append(side_a, a[i], section(i == 3 ? 1 : 3, i == 3)));
      ASSERT_TRUE(candidate->append(side_b, b[i], section(i == 3 ? 1 : 3, i == 3)));
      if (i != 3) relations.push_back({swap_sides ? &b[i] : &a[i],
                                       swap_sides ? &a[i] : &b[i], true});
    }
    HrnPositionPointSolver solver;
    const auto result = solver.solveCircular(*candidate, relations);
    ASSERT_TRUE(result.status.committed) << static_cast<int>(result.status.primary_reason);
    const auto& first = result.side(side_a);
    const auto& second = result.side(side_b);
    const auto distance = [](Point2D x, Point2D y) { return std::hypot(x.x - y.x, x.y - y.y); };
    for (size_t i = 0; i < 3; ++i)
      EXPECT_LE(distance(first.centers2d[i], second.centers2d[i]), 1e-9);
    for (const auto* side : {&first, &second}) {
      const bool first_side = side == &first;
      const double parent_radius = side->sections[3]->shape().dimensions().primary;
      const auto parent_center = side->centers2d[3];
      const size_t child = first_side ? 0 : 2;
      const size_t separate = first_side ? 2 : 0;
      EXPECT_NEAR(parent_radius, std::max(distance(parent_center, side->centers2d[child]),
                                         distance(parent_center, side->centers2d[1])) + 3, 1e-9);
      EXPECT_GE(distance(side->centers2d[child], side->centers2d[1]), 6 - 1e-9);
      EXPECT_GE(distance(parent_center, side->centers2d[separate]), parent_radius + 3 - 1e-9);
      EXPECT_LE(distance(parent_center, {0, 0}) + parent_radius, 20 + 1e-9);
      EXPECT_LE(distance(side->centers2d[separate], {0, 0}) + 3, 20 + 1e-9);
      EXPECT_DOUBLE_EQ(side->root->shape().dimensions().primary, 20);
    }
  }
}

TEST(HrnCircularTransaction, DistinctSharedCentersDoNotProveAConflict) {
  HrnSegment a, b, c[2], d[2];
  ASSERT_TRUE(c[0].attachTo(a));
  ASSERT_TRUE(c[1].attachTo(a));
  ASSERT_TRUE(d[0].attachTo(b));
  ASSERT_TRUE(d[1].attachTo(b));
  auto candidate = input(20);
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, a, section(6)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, b, section(6)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, c[0], section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, c[1], section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, d[0], section(2)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, d[1], section(3)));
  HrnPositionPointSolver solver;
  // Each side has a different initial child separation. Borrowing the
  // negative seed can keep both larger negative children non-overlapping.
  auto result = solver.solveCircular(*candidate, {{&c[0], &d[0], true}, {&c[1], &d[1], true}});
  ASSERT_TRUE(result.status.committed);
  EXPECT_GE(std::hypot(result.sides[0].centers2d[1].x - result.sides[0].centers2d[2].x,
                       result.sides[0].centers2d[1].y - result.sides[0].centers2d[2].y), 5 - 1e-9);
}

TEST(HrnCircularTransaction, RetriesWhenInitialSharedSeedCrossesFixedBoundary) {
  HrnSegment p, n, a[2], b[2];
  auto candidate = input(20);
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, p, section(6)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, n, section(5)));
  for (size_t i = 0; i < 2; ++i) {
    ASSERT_TRUE(a[i].attachTo(p));
    ASSERT_TRUE(b[i].attachTo(n));
    ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, a[i], section(i == 0 ? 3 : 1)));
    ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, b[i], section(i == 0 ? 1 : 3)));
  }
  HrnPositionPointSolver solver;
  const auto solved = solver.solveCircular(*candidate, {{&a[0], &b[0], true}, {&a[1], &b[1], true}});
  ASSERT_TRUE(solved.status.committed);
  for (size_t i : {1u, 2u}) {
    EXPECT_NEAR(solved.sides[0].centers2d[i].x, solved.sides[1].centers2d[i].x, 1e-9);
    EXPECT_NEAR(solved.sides[0].centers2d[i].y, solved.sides[1].centers2d[i].y, 1e-9);
    const auto center = solved.sides[1].centers2d[i];
    EXPECT_LE(std::hypot(center.x, center.y) + (i == 1 ? 1 : 3), 5 + 1e-9);
  }
}

TEST(HrnCircularTransaction, RepeatedEndpointAllowsOverlappingTopLevelMembers) {
  HrnPositionPointSolver solver;
  for (auto multi : {ZeHrnSide::Positive, ZeHrnSide::Negative}) {
    const auto single = multi == ZeHrnSide::Positive ? ZeHrnSide::Negative : ZeHrnSide::Positive;
    for (bool resize : {false, true}) {
      for (const auto& radii : {std::make_pair(3.0, 8.0), std::make_pair(8.0, 3.0),
                                std::make_pair(8.0, 8.0)}) {
        const double first_radius = radii.first, second_radius = radii.second;
        HrnSegment a, b, peer;
        Matrix4D matrix;
        matrix.m[0][3] = 7; matrix.m[1][3] = 8; matrix.m[2][3] = 9;
        auto candidate = HrnPositionPointInput::create(matrix, section(8, resize), section(8, resize));
        ASSERT_TRUE(candidate->append(multi, a, section(first_radius)));
        ASSERT_TRUE(candidate->append(multi, b, section(second_radius)));
        ASSERT_TRUE(candidate->append(single, peer, section(8)));
        const std::vector<HrnSegmentSideRelation> relations = multi == ZeHrnSide::Positive ?
            std::vector<HrnSegmentSideRelation>{{&a, &peer, true}, {&b, &peer, true}} :
            std::vector<HrnSegmentSideRelation>{{&peer, &a, true}, {&peer, &b, true}};
        HrnSectionTransactionResult status;
        auto point = HrnPositionPoint::createSolved(*candidate, relations, solver, status);
        ASSERT_TRUE(point) << static_cast<int>(status.primary_reason);
        EXPECT_TRUE(status.committed);
        EXPECT_TRUE(status.diagnostics.empty());
        EXPECT_EQ(point->sectionCount(multi), 2u);
        EXPECT_DOUBLE_EQ(point->sectionAt(multi, 0).shape().dimensions().primary, first_radius);
        EXPECT_DOUBLE_EQ(point->sectionAt(multi, 1).shape().dimensions().primary, second_radius);
        EXPECT_NE(&point->sectionAt(multi, 0), &point->sectionAt(multi, 1));
        EXPECT_NEAR(point->sideRootSection(multi).shape().dimensions().primary, 8, 1e-9);
        for (auto side : {multi, single}) {
          for (size_t i = 0; i < point->sectionCount(side); ++i) {
            const auto center = point->locationAt(side, i);
            EXPECT_NEAR(center.x, 7, 1e-9);
            EXPECT_NEAR(center.y, 8, 1e-9);
            EXPECT_NEAR(center.z, 9, 1e-9);
          }
        }
        // Growing an overlapping member beyond a fixed Root still rolls back.
        if (!resize) {
          auto failed = point->replaceSectionData(multi, a, section(9), solver);
          EXPECT_FALSE(failed.committed);
          EXPECT_DOUBLE_EQ(point->sectionAt(multi, 0).shape().dimensions().primary, first_radius);
          EXPECT_NEAR(point->locationAt(multi, 0).x, 7, 1e-9);
        }
      }
    }
  }
}

TEST(HrnCircularTransaction, MappedNestedSiblingsUseLargestEnvelopeAndKeepOtherClearance) {
  HrnPositionPointSolver solver;
  for (auto multi : {ZeHrnSide::Positive, ZeHrnSide::Negative}) {
    const auto single = multi == ZeHrnSide::Positive ? ZeHrnSide::Negative : ZeHrnSide::Positive;
    HrnSegment parent, a, b, other, peer;
    for (auto* child : {&a, &b, &other}) ASSERT_TRUE(child->attachTo(parent));
    auto candidate = input(20);
    auto layout = std::unique_ptr<HrnRuleBasedLayout>(new HrnRuleBasedLayout());
    ASSERT_TRUE(layout->setSpacingTolerance(1));
    auto parent_data = HrnSectionData::create(HrnCircularSectionShape::create(8, 1e-9),
        std::move(layout), HrnSectionOptions{});
    ASSERT_TRUE(candidate->append(multi, parent, std::move(parent_data)));
    ASSERT_TRUE(candidate->append(multi, a, section(2)));
    ASSERT_TRUE(candidate->append(multi, b, section(4)));
    ASSERT_TRUE(candidate->append(multi, other, section(1)));
    ASSERT_TRUE(candidate->append(single, peer, section(1)));
    const std::vector<HrnSegmentSideRelation> relations = multi == ZeHrnSide::Positive ?
        std::vector<HrnSegmentSideRelation>{{&a, &peer, true}, {&b, &peer, true},
                                           {&other, &peer, false}} :
        std::vector<HrnSegmentSideRelation>{{&peer, &a, true}, {&peer, &b, true},
                                           {&peer, &other, false}};
    const auto solved = solver.solveCircular(*candidate, relations);
    ASSERT_TRUE(solved.status.committed) << static_cast<int>(solved.status.primary_reason);
    const auto& side = solved.side(multi);
    EXPECT_NEAR(side.centers2d[1].x, side.centers2d[2].x, 1e-9);
    EXPECT_NEAR(side.centers2d[1].y, side.centers2d[2].y, 1e-9);
    EXPECT_NEAR(side.centers2d[1].x, solved.side(single).centers2d[0].x, 1e-9);
    EXPECT_NEAR(side.centers2d[1].y, solved.side(single).centers2d[0].y, 1e-9);
    EXPECT_GE(std::hypot(side.centers2d[2].x - side.centers2d[3].x,
                         side.centers2d[2].y - side.centers2d[3].y) + 1e-9, 6);
    for (size_t i : {1u, 2u, 3u})
      EXPECT_LE(std::hypot(side.centers2d[i].x, side.centers2d[i].y) +
          side.sections[i]->shape().dimensions().primary + 1, 8 + 1e-9);
    HrnSectionTransactionResult status;
    auto point = HrnPositionPoint::createSolved(*candidate, relations, solver, status);
    ASSERT_TRUE(point);
    const auto before = point->locationAt(multi, 1);
    const auto failed = point->replaceSectionData(multi, parent, section(3), solver);
    EXPECT_FALSE(failed.committed);
    EXPECT_DOUBLE_EQ(point->sectionAt(multi, 0).shape().dimensions().primary, 8);
    EXPECT_DOUBLE_EQ(point->locationAt(multi, 1).x, before.x);
    EXPECT_DOUBLE_EQ(point->locationAt(multi, 1).y, before.y);
  }
}

TEST(HrnCircularTransaction, TransitiveSamePositionClassAllowsOverlapOnBothSides) {
  HrnSegment p, q, n, m;
  auto candidate = input(4);
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, p, section(2)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, q, section(4)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, n, section(3)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, m, section(1)));
  HrnPositionPointSolver solver;
  const auto solved = solver.solveCircular(*candidate,
      {{&p, &n, true}, {&q, &n, true}, {&q, &m, true}});
  ASSERT_TRUE(solved.status.committed);
  for (const auto& side : solved.sides) {
    for (const auto& center : side.centers2d) {
      EXPECT_NEAR(center.x, 0, 1e-9);
      EXPECT_NEAR(center.y, 0, 1e-9);
    }
  }
}

TEST(HrnCircularTransaction, OrdinaryRepeatedEndpointDoesNotAllowOverlapInTightRoot) {
  HrnSegment p, q, n;
  auto candidate = input(8);
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, p, section(8)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, q, section(8)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, n, section(8)));
  HrnPositionPointSolver solver;
  for (const auto& relations : {
      std::vector<HrnSegmentSideRelation>{{&p, &n, false}, {&q, &n, false}},
      std::vector<HrnSegmentSideRelation>{{&p, &n, true}, {&q, &n, false}}}) {
    const auto solved = solver.solveCircular(*candidate, relations);
    EXPECT_FALSE(solved.status.committed);
    EXPECT_EQ(solved.status.primary_reason, HrnSectionFailureReason::FixedBoundaryInsufficient);
  }
}

TEST(HrnCircularTransaction, RecompactsDerangedFamilyAroundMovedMappedCenters) {
  HrnPositionPointSolver solver;
  for (auto crowded : {ZeHrnSide::Positive, ZeHrnSide::Negative}) {
    const auto peer_side = crowded == ZeHrnSide::Positive ? ZeHrnSide::Negative : ZeHrnSide::Positive;
    for (double root_rotation : {0.0, 90.0}) {
    HrnSegment nodes[10], peers[2], child, other_child, grandchild, other_grandchild;
    auto candidate = HrnPositionPointInput::create(Matrix4D{},
        section(20, false, crowded == ZeHrnSide::Positive ? root_rotation : 0),
        section(20, false, crowded == ZeHrnSide::Negative ? root_rotation : 0));
    for (size_t i = 0; i < 10; ++i)
      ASSERT_TRUE(candidate->append(crowded, nodes[i], section(i == 0 ? 8 : 1, false, i == 0 ? 30 : 0)));
    ASSERT_TRUE(child.attachTo(nodes[0]));
    ASSERT_TRUE(other_child.attachTo(nodes[0]));
    ASSERT_TRUE(grandchild.attachTo(child));
    ASSERT_TRUE(other_grandchild.attachTo(child));
    ASSERT_TRUE(candidate->append(crowded, child, section(2, false, 15)));
    ASSERT_TRUE(candidate->append(crowded, grandchild, section(0.5)));
    ASSERT_TRUE(candidate->append(crowded, other_child, section(1)));
    ASSERT_TRUE(candidate->append(crowded, other_grandchild, section(0.5)));
    ASSERT_TRUE(candidate->append(peer_side, peers[0], section(8)));
    ASSERT_TRUE(candidate->append(peer_side, peers[1], section(5)));
    const std::vector<HrnSegmentSideRelation> relations = crowded == ZeHrnSide::Positive ?
        std::vector<HrnSegmentSideRelation>{{&nodes[4], &peers[0], true}, {&nodes[5], &peers[1], true}} :
        std::vector<HrnSegmentSideRelation>{{&peers[0], &nodes[4], true}, {&peers[1], &nodes[5], true}};
    const auto solved = solver.solveCircular(*candidate, relations);
    ASSERT_TRUE(solved.status.committed);
    const auto& side = solved.side(crowded);
    const auto& peer = solved.side(peer_side);
    const auto independent = solver.solveCircular(*candidate, {});
    ASSERT_TRUE(independent.status.committed);
    for (size_t descendant : {10u, 11u, 12u, 13u}) {
      EXPECT_NEAR(side.centers2d[descendant].x - side.centers2d[0].x,
          independent.side(crowded).centers2d[descendant].x -
              independent.side(crowded).centers2d[0].x, 1e-9);
      EXPECT_NEAR(side.centers2d[descendant].y - side.centers2d[0].y,
          independent.side(crowded).centers2d[descendant].y -
              independent.side(crowded).centers2d[0].y, 1e-9);
      EXPECT_DOUBLE_EQ(side.absolute_rotations[descendant],
                       independent.side(crowded).absolute_rotations[descendant]);
    }
    double envelope = 0;
    for (size_t i = 0; i < 10; ++i) {
      const auto a = side.centers2d[i];
      const double radius = i == 0 ? 8 : 1;
      envelope = std::max(envelope, std::hypot(a.x, a.y) + radius);
      double nearest_gap = 100;
      for (size_t j = 0; j < 10; ++j) {
        if (i == j) continue;
        const auto b = side.centers2d[j];
        const double gap = std::hypot(a.x - b.x, a.y - b.y) - radius - (j == 0 ? 8 : 1);
        EXPECT_GE(gap + 1e-9, 0);
        nearest_gap = std::min(nearest_gap, std::fabs(gap));
      }
      // Every free member should join the tangent packing, not remain isolated.
      if (i != 4 && i != 5) EXPECT_LE(nearest_gap, 1e-7);
    }
    EXPECT_LE(envelope, 15);
    const auto main = side.centers2d[0];
    for (size_t i : {4u, 5u})
      EXPECT_NEAR(std::hypot(main.x - side.centers2d[i].x,
                            main.y - side.centers2d[i].y), 9, 1e-9);
    for (size_t i = 0; i < 2; ++i) {
      EXPECT_NEAR(side.centers2d[i + 4].x, peer.centers2d[i].x, 1e-9);
      EXPECT_NEAR(side.centers2d[i + 4].y, peer.centers2d[i].y, 1e-9);
    }
    const auto repeat = solver.solveCircular(*candidate, relations);
    ASSERT_TRUE(repeat.status.committed);
    for (size_t i = 0; i < 10; ++i) {
      EXPECT_DOUBLE_EQ(side.centers2d[i].x, repeat.side(crowded).centers2d[i].x);
      EXPECT_DOUBLE_EQ(side.centers2d[i].y, repeat.side(crowded).centers2d[i].y);
    }
    HrnSectionTransactionResult status;
    auto point = HrnPositionPoint::createSolved(*candidate, relations, solver, status);
    ASSERT_TRUE(point);
    std::vector<Point3D> before;
    for (size_t i = 0; i < point->sectionCount(crowded); ++i)
      before.push_back(point->locationAt(crowded, i));
    const auto failed = point->replaceSideRootSectionData(crowded, section(13.5), solver);
    EXPECT_FALSE(failed.committed);
    EXPECT_NE(failed.primary_reason, HrnSectionFailureReason::GeometryValidationFailed);
    EXPECT_DOUBLE_EQ(point->sideRootSection(crowded).shape().dimensions().primary, 20);
    for (size_t i = 0; i < before.size(); ++i) {
      EXPECT_DOUBLE_EQ(point->locationAt(crowded, i).x, before[i].x);
      EXPECT_DOUBLE_EQ(point->locationAt(crowded, i).y, before[i].y);
    }
    }
  }
}

TEST(HrnCircularTransaction, CoupledResizeRechecksAncestorSiblings) {
  HrnSegment parent, sibling, child_a, child_b, negative_root;
  ASSERT_TRUE(child_a.attachTo(parent));
  ASSERT_TRUE(child_b.attachTo(parent));
  auto candidate = input(5);
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, parent, section(1, true)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, sibling, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, child_a, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, child_b, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, negative_root, section(1)));
  HrnPositionPointSolver solver;
  HrnSectionTransactionResult status;
  auto point = HrnPositionPoint::createSolved(*candidate,
      {{&child_a, &negative_root, true}}, solver, status);
  ASSERT_TRUE(point);
  ASSERT_TRUE(status.committed);
  const auto parent_center = point->locationAt(ZeHrnSide::Positive, 0);
  const auto sibling_center = point->locationAt(ZeHrnSide::Positive, 1);
  const double parent_radius = point->sectionAt(ZeHrnSide::Positive, 0).shape().dimensions().primary;
  EXPECT_GE(std::hypot(parent_center.x - sibling_center.x,
                       parent_center.y - sibling_center.y) + 1e-9,
            parent_radius + 1);
  EXPECT_DOUBLE_EQ(point->locationAt(ZeHrnSide::Positive, 2).x,
                   point->locationAt(ZeHrnSide::Negative, 0).x);
}

TEST(HrnCircularTransaction, SolverBudgetFailureHasNodeDiagnostic) {
  HrnSegment a, b;
  HrnSectionOptions options;
  std::unique_ptr<HrnRuleBasedLayout> limited(new HrnRuleBasedLayout());
  ASSERT_TRUE(limited->setSolveSettings(HrnLayoutSolveSettings(1, 1, 1e-9)));
  auto root = HrnSectionData::create(HrnCircularSectionShape::create(10, 1e-9),
                                     std::move(limited), options);
  auto candidate = HrnPositionPointInput::create(Matrix4D(), std::move(root), section(10));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, a, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, b, section(1)));
  HrnPositionPointSolver solver;
  HrnSectionTransactionResult status;
  auto point = HrnPositionPoint::createSolved(*candidate, {}, solver, status);
  EXPECT_FALSE(point);
  EXPECT_EQ(status.primary_reason, HrnSectionFailureReason::IterationBudgetExceeded);
  ASSERT_EQ(status.diagnostics.size(), 1u);
  EXPECT_EQ(status.diagnostics[0].stage, HrnSectionFailureStage::LayoutSolve);
  EXPECT_EQ(status.diagnostics[0].iterations_used, 1u);
  EXPECT_EQ(status.diagnostics[0].algorithm_version, 1u);
  ASSERT_FALSE(status.diagnostics[0].adjustments.empty());
  EXPECT_EQ(status.diagnostics[0].adjustments[0].parameter,
            HrnSectionParameterKey::MaxIterations);
}

TEST(HrnCircularTransaction, RepackingFindsPreviouslyMissedNestedCandidate) {
  HrnSegment parent, sibling, child_a, child_b, negative_root, negative_child;
  ASSERT_TRUE(child_a.attachTo(parent));
  ASSERT_TRUE(child_b.attachTo(parent));
  ASSERT_TRUE(negative_child.attachTo(negative_root));
  auto candidate = input(4.9);
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, parent, section(1, true)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, sibling, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, child_a, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, child_b, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, negative_root, section(1)));
  // A single top-level root with a child must still stay anchored.
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, negative_child, section(0.5)));
  HrnPositionPointSolver solver;
  HrnSectionTransactionResult status;
  auto point = HrnPositionPoint::createSolved(*candidate,
      {{&child_a, &negative_root, true}}, solver, status);
  ASSERT_TRUE(point);
  EXPECT_TRUE(status.committed);
  EXPECT_TRUE(status.diagnostics.empty());
  const auto parent_center = point->locationAt(ZeHrnSide::Positive, 0);
  const auto sibling_center = point->locationAt(ZeHrnSide::Positive, 1);
  const double radius = point->sectionAt(ZeHrnSide::Positive, 0).shape().dimensions().primary;
  EXPECT_GE(std::hypot(parent_center.x - sibling_center.x,
                       parent_center.y - sibling_center.y) + 1e-9, radius + 1);
  EXPECT_LE(std::hypot(parent_center.x, parent_center.y) + radius, 4.9 + 1e-9);
  EXPECT_NEAR(point->locationAt(ZeHrnSide::Positive, 2).x, 0, 1e-9);
  EXPECT_NEAR(point->locationAt(ZeHrnSide::Positive, 2).y, 0, 1e-9);
}

TEST(HrnCircularTransaction, CandidateSpaceFailureIsNotHardConflict) {
  HrnSegment parent, child_a, child_b, negative_root, negative_child;
  ASSERT_TRUE(child_a.attachTo(parent));
  ASSERT_TRUE(child_b.attachTo(parent));
  ASSERT_TRUE(negative_child.attachTo(negative_root));
  auto candidate = input(20);
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, parent, section(4)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, child_a, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, child_b, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, negative_root, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, negative_child, section(0.5)));
  HrnPositionPointSolver solver;
  HrnSectionTransactionResult status;
  // Distinct classes use coincident seeds; they cannot receive the overlap exemption.
  auto point = HrnPositionPoint::createSolved(*candidate,
      {{&child_a, &negative_root, true}, {&child_b, &negative_child, true}}, solver, status);
  EXPECT_FALSE(point);
  EXPECT_EQ(status.primary_reason, HrnSectionFailureReason::NoSolutionFound);
  ASSERT_FALSE(status.diagnostics.empty());
  ASSERT_FALSE(status.diagnostics.back().conflicts.empty());
  EXPECT_EQ(status.diagnostics.back().conflicts.front().kind, HrnSectionConflictKind::Overlap);
}

TEST(HrnCircularTransaction, UnrelatedLeafBudgetDoesNotLimitCoupledGroup) {
  HrnSegment parent, sibling, child_a, child_b, negative_root, unrelated_leaf;
  ASSERT_TRUE(child_a.attachTo(parent));
  ASSERT_TRUE(child_b.attachTo(parent));
  ASSERT_TRUE(unrelated_leaf.attachTo(negative_root));
  auto candidate = input(5);
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, parent, section(1, true)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, sibling, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, child_a, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, child_b, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, negative_root, section(1)));
  HrnSectionOptions options;
  std::unique_ptr<HrnRuleBasedLayout> limited(new HrnRuleBasedLayout());
  ASSERT_TRUE(limited->setSolveSettings(HrnLayoutSolveSettings(1, 1, 1e-9)));
  auto leaf = HrnSectionData::create(HrnCircularSectionShape::create(0.5, 1e-9),
                                      std::move(limited), options);
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, unrelated_leaf, std::move(leaf)));
  HrnPositionPointSolver solver;
  HrnSectionTransactionResult status;
  auto point = HrnPositionPoint::createSolved(*candidate,
      {{&child_a, &negative_root, true}}, solver, status);
  EXPECT_TRUE(point);
  EXPECT_TRUE(status.committed);
}

TEST(HrnCircularTransaction, RejectsAliasedSectionPassedToEdit) {
  EXPECT_EXIT({
    HrnSegment segment;
    auto candidate = input();
    if (!candidate->append(ZeHrnSide::Positive, segment, section(1))) std::_Exit(3);
    HrnPositionPointSolver solver;
    HrnSectionTransactionResult status;
    auto point = HrnPositionPoint::createSolved(*candidate, {}, solver, status);
    if (!point) std::_Exit(4);
    HrnSectionData* borrowed = const_cast<HrnSectionData*>(
        &point->sectionAt(ZeHrnSide::Positive, 0));
    const auto edit = point->replaceSectionData(ZeHrnSide::Positive, segment,
        std::unique_ptr<HrnSectionData>(borrowed), solver);
    if (!edit.committed && edit.primary_reason == HrnSectionFailureReason::InvalidInput &&
        &point->sectionAt(ZeHrnSide::Positive, 0) == borrowed) std::_Exit(0);
    std::_Exit(5);
  }, ::testing::ExitedWithCode(0), "");
}
