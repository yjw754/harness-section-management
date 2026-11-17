#include <gtest/gtest.h>

#include <cmath>

#include "hrn/section/HrnRuleBasedLayoutSolver.h"
#include "hrn/section/HrnSectionGeometryService.h"
#include "fixtures/circular_rule_layout_cases.h"

using namespace hrn;

namespace {
HrnCircularLayoutRequest request(std::initializer_list<double> radii, double spacing = 0) {
  HrnCircularLayoutRequest result;
  result.spacing = spacing;
  size_t id = 0;
  for (double radius : radii) result.members.push_back({id++, radius});
  return result;
}
}

TEST(HrnCircularRuleLayout, OneTwoThreeCirclesHaveFrozenCenters) {
  HrnCircularRuleBasedLayoutSolver solver;
  auto one = solver.solve(request({1}));
  ASSERT_TRUE(one.succeeded);
  ASSERT_EQ(one.placements.size(), 1u);
  EXPECT_DOUBLE_EQ(one.placements[0].center.x, 0);
  EXPECT_DOUBLE_EQ(one.placements[0].center.y, 0);
  EXPECT_DOUBLE_EQ(one.required_parent_radius, 1);

  auto two = solver.solve(request({1, 1}));
  ASSERT_TRUE(two.succeeded);
  EXPECT_DOUBLE_EQ(two.placements[1].center.x, 2);
  EXPECT_DOUBLE_EQ(two.placements[1].center.y, 0);
  EXPECT_DOUBLE_EQ(two.required_parent_radius, 3);

  auto three = solver.solve(request({1, 1, 1}));
  ASSERT_TRUE(three.succeeded);
  EXPECT_NEAR(three.placements[2].center.x, 1, 1e-9);
  EXPECT_NEAR(three.placements[2].center.y, std::sqrt(3.0), 1e-9);
  EXPECT_NEAR(three.required_parent_radius, 3, 1e-9);
}

TEST(HrnCircularRuleLayout, SortsStablyAndHonorsSpacing) {
  HrnCircularRuleBasedLayoutSolver solver;
  auto result = solver.solve(request({1, 2, 1}, 0.5));
  ASSERT_TRUE(result.succeeded);
  ASSERT_EQ(result.placements.size(), 3u);
  EXPECT_EQ(result.placements[0].id, 1u);
  EXPECT_EQ(result.placements[1].id, 0u);
  EXPECT_EQ(result.placements[2].id, 2u);
  EXPECT_DOUBLE_EQ(result.placements[0].center.x, 0);
  EXPECT_DOUBLE_EQ(result.placements[1].center.x, 3.5);
  HrnCircularGeometryService geometry;
  for (size_t i = 0; i < result.placements.size(); ++i) {
    for (size_t j = i + 1; j < result.placements.size(); ++j) {
      auto distance = geometry.signedDistance(
          {result.placements[i].center, result.placements[i].radius},
          {result.placements[j].center, result.placements[j].radius}, 1e-9);
      ASSERT_TRUE(distance.valid);
      EXPECT_GE(distance.signed_distance + 1e-9, 0.5);
    }
  }
}

TEST(HrnCircularRuleLayout, SixCirclesAndRepeatability) {
  HrnCircularRuleBasedLayoutSolver solver;
  auto input = request({1, 1, 1, 1, 1, 1});
  auto first = solver.solve(input);
  ASSERT_TRUE(first.succeeded);
  ASSERT_EQ(first.placements.size(), 6u);
  for (int repeat = 0; repeat < 100; ++repeat) {
    auto result = solver.solve(input);
    ASSERT_TRUE(result.succeeded);
    ASSERT_EQ(result.candidate_count, first.candidate_count);
    ASSERT_EQ(result.iterations_used, first.iterations_used);
    ASSERT_DOUBLE_EQ(result.required_parent_radius, first.required_parent_radius);
    for (size_t i = 0; i < first.placements.size(); ++i) {
      EXPECT_EQ(result.placements[i].id, first.placements[i].id);
      EXPECT_DOUBLE_EQ(result.placements[i].center.x, first.placements[i].center.x);
      EXPECT_DOUBLE_EQ(result.placements[i].center.y, first.placements[i].center.y);
    }
  }
}

TEST(HrnCircularRuleLayout, SevenFrozenReferenceVectors) {
  HrnCircularRuleBasedLayoutSolver solver;
  for (const auto& frozen : hrn_test::circularRuleCases()) {
    HrnCircularLayoutRequest input;
    input.spacing = frozen.spacing;
    for (size_t i = 0; i < frozen.radii.size(); ++i)
      input.members.push_back({i, frozen.radii[i]});
    const auto result = solver.solve(input);
    ASSERT_TRUE(result.succeeded);
    ASSERT_EQ(result.placements.size(), frozen.centers.size());
    EXPECT_NEAR(result.required_parent_radius, frozen.parent_radius, 1e-9);
    EXPECT_EQ(result.candidate_count, frozen.candidates);
    EXPECT_EQ(result.iterations_used, frozen.iterations);
    for (size_t i = 0; i < frozen.centers.size(); ++i) {
      EXPECT_EQ(result.placements[i].id, frozen.centers[i].id);
      EXPECT_NEAR(result.placements[i].center.x, frozen.centers[i].x, 1e-9);
      EXPECT_NEAR(result.placements[i].center.y, frozen.centers[i].y, 1e-9);
    }
  }
}

TEST(HrnCircularRuleLayout, RejectsUnknownVersionAndHonorsBudget) {
  HrnCircularRuleBasedLayoutSolver solver;
  auto input = request({1, 1, 1});
  input.settings.algorithm_version = 99;
  EXPECT_EQ(solver.solve(input).reason, HrnSectionFailureReason::InvalidInput);
  input.settings.algorithm_version = HrnCircularRuleBasedLayoutSolver::kAlgorithmVersion;
  input.settings.max_iterations = 1;
  auto budget = solver.solve(input);
  EXPECT_FALSE(budget.succeeded);
  EXPECT_EQ(budget.reason, HrnSectionFailureReason::IterationBudgetExceeded);
  EXPECT_EQ(budget.iterations_used, 1u);
  input.settings.max_iterations = 1000;
  input.members[0].radius = -1;
  EXPECT_EQ(solver.solve(input).reason, HrnSectionFailureReason::InvalidInput);
}
