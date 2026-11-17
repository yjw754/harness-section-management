#include <gtest/gtest.h>

#include <cmath>
#include <cstdlib>

#include "hrn/section/HrnPositionPoint.h"
#include "hrn/section/HrnPositionPointSolver.h"

using namespace hrn;
namespace {
std::unique_ptr<HrnSectionData> section(double radius, bool resize = false) {
  HrnSectionOptions options;
  options.auto_resize_enabled = resize;
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

TEST(HrnCircularTransaction, IncompatibleConnectedClassHasConflictWitness) {
  HrnSegment p, q, n;
  auto candidate = input(10);
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, p, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, q, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, n, section(1)));
  HrnPositionPointSolver solver;
  HrnSectionTransactionResult status;
  auto point = HrnPositionPoint::createSolved(*candidate,
      {{&p, &n, true}, {&q, &n, true}}, solver, status);
  EXPECT_FALSE(point);
  EXPECT_FALSE(status.committed);
  EXPECT_EQ(status.primary_reason, HrnSectionFailureReason::SamePositionConstraintConflict);
  ASSERT_FALSE(status.diagnostics.empty());
  ASSERT_FALSE(status.diagnostics.front().conflicts.empty());
  EXPECT_TRUE(status.diagnostics.front().conflicts.front().has_lhs_witness);
  EXPECT_TRUE(status.diagnostics.front().conflicts.front().has_rhs_witness);
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

TEST(HrnCircularTransaction, CandidateSpaceFailureIsNotHardConflict) {
  HrnSegment parent, sibling, child_a, child_b, negative_root;
  ASSERT_TRUE(child_a.attachTo(parent));
  ASSERT_TRUE(child_b.attachTo(parent));
  auto candidate = input(4.9);
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, parent, section(1, true)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, sibling, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, child_a, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, child_b, section(1)));
  ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, negative_root, section(1)));
  HrnPositionPointSolver solver;
  HrnSectionTransactionResult status;
  auto point = HrnPositionPoint::createSolved(*candidate,
      {{&child_a, &negative_root, true}}, solver, status);
  EXPECT_FALSE(point);
  EXPECT_EQ(status.primary_reason, HrnSectionFailureReason::NoSolutionFound);
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
