#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>

#include "hrn/section/HrnPositionPoint.h"
#include "hrn/section/HrnPositionPointSolver.h"
#include "hrn/section/HrnSectionGeometryService.h"
#include "HrnSectionCaseBuilder.h"

using namespace hrn;
namespace {
std::unique_ptr<HrnSectionData> section(double radius, bool resize) {
  HrnSectionOptions options;
  options.auto_resize_enabled = resize;
  return HrnSectionData::create(HrnCircularSectionShape::create(radius, 1e-9),
      std::unique_ptr<HrnSectionLayout>(new HrnRuleBasedLayout()), options);
}
void independentlyCheckSide(const HrnCircularSolvedSide& side,
                            const std::vector<std::pair<size_t, size_t>>& shared_pairs = {}) {
  HrnCircularGeometryService geometry;
  std::map<const HrnSegment*, size_t> index;
  for (size_t i = 0; i < side.segments.size(); ++i) index[side.segments[i]] = i;
  std::vector<std::vector<size_t>> children(side.segments.size());
  std::vector<size_t> roots;
  for (size_t i = 0; i < side.segments.size(); ++i) {
    const auto parent = index.find(side.segments[i]->parent());
    if (parent == index.end()) roots.push_back(i);
    else children[parent->second].push_back(i);
  }
  const auto checkFamily = [&](const std::vector<size_t>& members, Point2D parent_center,
                               double parent_radius, double spacing) {
    for (size_t i = 0; i < members.size(); ++i) {
      const auto& a = side.centers2d[members[i]];
      const double ar = side.sections[members[i]]->shape().dimensions().primary;
      const auto containment = geometry.containsWithClearance(
          {parent_center, parent_radius}, {a, ar}, spacing, 1e-9);
      ASSERT_TRUE(containment.valid);
      EXPECT_TRUE(containment.contained);
      for (size_t j = i + 1; j < members.size(); ++j) {
        bool shared = false;
        for (const auto& pair : shared_pairs)
          shared = shared || (pair.first == members[i] && pair.second == members[j]) ||
                             (pair.second == members[i] && pair.first == members[j]);
        if (shared) {
          EXPECT_NEAR(a.x, side.centers2d[members[j]].x, 1e-9);
          EXPECT_NEAR(a.y, side.centers2d[members[j]].y, 1e-9);
          continue;
        }
        const auto& b = side.centers2d[members[j]];
        const double br = side.sections[members[j]]->shape().dimensions().primary;
        const auto distance = geometry.signedDistance({a, ar}, {b, br}, 1e-9);
        ASSERT_TRUE(distance.valid);
        EXPECT_GE(distance.signed_distance + 1e-9, spacing);
      }
    }
  };
  for (size_t i = 0; i < children.size(); ++i)
    if (!children[i].empty())
      checkFamily(children[i], side.centers2d[i],
          side.sections[i]->shape().dimensions().primary,
          side.sections[i]->layout().spacingTolerance());
  if (roots.size() > 1)
    checkFamily(roots, {0, 0}, side.root->shape().dimensions().primary,
                side.root->layout().spacingTolerance());
}
}

TEST(Stage2Acceptance, HundredCirclesFiveLevelsBothSidesAndRepeatability) {
  Matrix4D matrix;
  matrix.m[0][3] = 10;
  matrix.m[1][3] = 20;
  auto candidate = HrnPositionPointInput::create(matrix, section(100, true), section(100, true));
  ASSERT_TRUE(candidate);
  std::vector<std::unique_ptr<HrnSegment>> graph;
  for (int i = 0; i < 100; ++i) graph.emplace_back(new HrnSegment());
  for (int i = 1; i < 100; ++i)
    ASSERT_TRUE(graph[i]->attachTo(*graph[(i - 1) / 3]));
  for (int i = 0; i < 100; ++i)
    ASSERT_TRUE(candidate->append(ZeHrnSide::Positive, *graph[i], section(1, true)));
  for (int i = 50; i < 100; ++i)
    ASSERT_TRUE(candidate->append(ZeHrnSide::Negative, *graph[i], section(1, true)));
  HrnPositionPointSolver solver;
  const auto started = std::chrono::steady_clock::now();
  auto first = solver.solveCircular(*candidate, {});
  const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now() - started).count();
  ASSERT_TRUE(first.status.committed);
  ASSERT_EQ(first.side(ZeHrnSide::Positive).locations.size(), 100u);
  ASSERT_EQ(first.side(ZeHrnSide::Negative).locations.size(), 50u);
  independentlyCheckSide(first.side(ZeHrnSide::Positive));
  independentlyCheckSide(first.side(ZeHrnSide::Negative));
  auto repeated = solver.solveCircular(*candidate, {});
  ASSERT_TRUE(repeated.status.committed);
  for (const auto side : {ZeHrnSide::Positive, ZeHrnSide::Negative}) {
    const auto& a = first.side(side);
    const auto& b = repeated.side(side);
    EXPECT_EQ(a.candidate_count, b.candidate_count);
    EXPECT_EQ(a.iterations_used, b.iterations_used);
    for (size_t i = 0; i < a.locations.size(); ++i) {
      EXPECT_DOUBLE_EQ(a.locations[i].x, b.locations[i].x);
      EXPECT_DOUBLE_EQ(a.locations[i].y, b.locations[i].y);
      EXPECT_DOUBLE_EQ(a.sections[i]->shape().dimensions().primary,
                       b.sections[i]->shape().dimensions().primary);
    }
  }
  HrnSectionTransactionResult status;
  auto point = HrnPositionPoint::createSolved(*candidate, {}, solver, status);
  ASSERT_TRUE(point);
  const auto edit_started = std::chrono::steady_clock::now();
  auto edit = point->replaceSectionData(ZeHrnSide::Positive, *graph[99], section(1.01, true), solver);
  const auto edit_duration = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now() - edit_started).count();
  ASSERT_TRUE(edit.committed);
  std::cout << "stage2_benchmark solve_us=" << duration << " edit_us=" << edit_duration
            << " candidates=" << first.side(ZeHrnSide::Positive).candidate_count +
                first.side(ZeHrnSide::Negative).candidate_count
            << " iterations=" << first.side(ZeHrnSide::Positive).iterations_used +
                first.side(ZeHrnSide::Negative).iterations_used << std::endl;
}

TEST(Stage2Acceptance, FiveLevelMappedCaseAllowsMappedSiblingOverlap) {
  Matrix4D matrix;
  matrix.m[0][0] = 0; matrix.m[0][1] = -1;
  matrix.m[1][0] = 1; matrix.m[1][1] = 0;
  matrix.m[0][3] = 10; matrix.m[1][3] = 20; matrix.m[2][3] = 30;
  HrnCircularSectionCaseOptions root;
  root.radius = 20;
  root.auto_resize_enabled = true;
  HrnCircularSectionCaseOptions member;
  member.radius = 2;
  member.auto_resize_enabled = true;
  member.spacing = 0.2;
  member.solve_settings = HrnLayoutSolveSettings(1, 100, 1e-8);
  HrnSectionCaseBuilder builder;
  builder.positionTransform(matrix).modelingTolerance(1e-6)
      .segment("P1").segment("P2").segment("P3").segment("P4")
      .segment("P5").segment("P6").segment("N1").segment("N2").segment("N3")
      .parent("P2", "P1").parent("P3", "P2").parent("P4", "P3")
      .parent("P5", "P4")
      .sideRootCircular(ZeHrnSide::Positive, root)
      .sideRootCircular(ZeHrnSide::Negative, root);
  for (const char* label : {"P1", "P2", "P3", "P4", "P5", "P6"})
    builder.circularSection(ZeHrnSide::Positive, label, member);
  for (const char* label : {"N1", "N2", "N3"})
    builder.circularSection(ZeHrnSide::Negative, label, member);
  // Two negative siblings intentionally share one center through P1.
  builder.relate("P1", "N1", true).relate("P1", "N3", true);
  auto case_data = builder.build();
  ASSERT_TRUE(case_data.succeeded);
  HrnPositionPointSolver solver;
  HrnSectionTransactionResult status;
  auto point = HrnPositionPoint::createSolved(*case_data.input,
      case_data.relations, solver, status);
  ASSERT_TRUE(point) << static_cast<int>(status.primary_reason);
  EXPECT_TRUE(status.committed);
  EXPECT_TRUE(status.diagnostics.empty());
  const auto solved = solver.solveCircular(*case_data.input, case_data.relations);
  ASSERT_TRUE(solved.status.committed);
  independentlyCheckSide(solved.sides[0]);
  independentlyCheckSide(solved.sides[1], {{0, 2}});
  for (size_t negative : {0u, 2u}) {
    EXPECT_NEAR(solved.sides[0].centers2d[0].x, solved.sides[1].centers2d[negative].x, 1e-9);
    EXPECT_NEAR(solved.sides[0].centers2d[0].y, solved.sides[1].centers2d[negative].y, 1e-9);
    EXPECT_NEAR(point->locationAt(ZeHrnSide::Positive, 0).x,
                point->locationAt(ZeHrnSide::Negative, negative).x, 1e-9);
    EXPECT_NEAR(point->locationAt(ZeHrnSide::Positive, 0).y,
                point->locationAt(ZeHrnSide::Negative, negative).y, 1e-9);
  }
}
