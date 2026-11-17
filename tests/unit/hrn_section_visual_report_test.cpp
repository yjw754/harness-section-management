#include <gtest/gtest.h>

#include "HrnSectionVisualReport.h"
#include "HrnSectionCaseBuilder.h"

using namespace hrn;
namespace {
std::unique_ptr<HrnSectionData> circle(double radius) {
  HrnSectionOptions options;
  return HrnSectionData::create(HrnCircularSectionShape::create(radius, 1e-9),
      std::unique_ptr<HrnSectionLayout>(new HrnRuleBasedLayout()), options);
}
}
TEST(HrnVisualReport, BilateralIdentityMappingAndSolverValues) {
  Matrix4D matrix;
  HrnSegment shared, other;
  auto input = HrnPositionPointInput::create(matrix, circle(10), circle(10));
  ASSERT_TRUE(input->append(ZeHrnSide::Positive, shared, circle(1)));
  ASSERT_TRUE(input->append(ZeHrnSide::Positive, other, circle(1)));
  ASSERT_TRUE(input->append(ZeHrnSide::Negative, shared, circle(1)));
  HrnPositionPointSolver solver;
  auto solved = solver.solveCircular(*input, {{&shared, &shared, true}});
  ASSERT_TRUE(solved.status.committed);
  auto model = buildVisualReportModel(*input, {{&shared, &shared, true}}, solved,
      [](const HrnSegment&) { return std::string("A<&\"</script>"); });
  ASSERT_TRUE(model.valid);
  ASSERT_EQ(model.nodes.size(), 3u);
  ASSERT_EQ(model.relations.size(), 1u);
  EXPECT_NE(model.relations[0].positive_id, model.relations[0].negative_id);
  EXPECT_TRUE(model.relations[0].same_position);
  EXPECT_DOUBLE_EQ(model.nodes[0].center2d.x, solved.side(ZeHrnSide::Positive).centers2d[0].x);
  EXPECT_DOUBLE_EQ(model.nodes[0].solved_radius,
                   solved.side(ZeHrnSide::Positive).sections[0]->shape().dimensions().primary);
  EXPECT_DOUBLE_EQ(model.nodes[0].solved_rotation,
                   solved.side(ZeHrnSide::Positive).absolute_rotations[0]);
  const std::string html = renderVisualReportHtml(model);
  EXPECT_NE(html.find("A&lt;&amp;&quot;&lt;/script&gt;"), std::string::npos);
  EXPECT_EQ(html.find("A<&\"</script>"), std::string::npos);
  EXPECT_NE(html.find("data-node=\"p0\""), std::string::npos);
  EXPECT_NE(html.find("data-node=\"n0\""), std::string::npos);
  EXPECT_NE(html.find("data-action=\"zoom-in\""), std::string::npos);
  EXPECT_NE(html.find("})();</script>"), std::string::npos);
  EXPECT_NE(html.find("class=\"side-root\""), std::string::npos);
  EXPECT_EQ(html.find("https://"), std::string::npos);
}
TEST(HrnVisualReport, RejectsMissingEndpointAndHidesFailedGeometry) {
  Matrix4D matrix;
  HrnSegment positive, missing, negative;
  auto input = HrnPositionPointInput::create(matrix, circle(10), circle(10));
  ASSERT_TRUE(input->append(ZeHrnSide::Positive, positive, circle(1)));
  ASSERT_TRUE(input->append(ZeHrnSide::Negative, negative, circle(1)));
  HrnPositionPointSolver solver;
  auto solved = solver.solveCircular(*input, {});
  auto bad = buildVisualReportModel(*input, {{&positive, &missing, false}}, solved,
      [](const HrnSegment&) { return std::string("x"); });
  EXPECT_FALSE(bad.valid);
  solved.status.committed = false;
  solved.status.primary_reason = HrnSectionFailureReason::SamePositionConstraintConflict;
  auto failed = buildVisualReportModel(*input, {{&positive, &negative, true}}, solved,
      [](const HrnSegment&) { return std::string("x"); });
  ASSERT_TRUE(failed.valid);
  EXPECT_FALSE(failed.solved);
  for (const auto& node : failed.nodes) EXPECT_FALSE(node.has_geometry);
  const std::string html = renderVisualReportHtml(failed);
  EXPECT_EQ(html.find("class=\"section-circle\""), std::string::npos);
  EXPECT_NE(html.find("SamePositionConstraintConflict"), std::string::npos);
}
TEST(HrnVisualReport, FiveLevelTreeKeepsParentAndMappingKinds) {
  auto sample = makeFiveLevelMappedCircularCase();
  ASSERT_TRUE(sample.succeeded);
  HrnPositionPointSolver solver;
  auto result = solver.solveCircular(*sample.input, sample.relations);
  auto model = buildVisualReportModel(*sample.input, sample.relations, result,
      [&](const HrnSegment& segment) { return sample.graph.labelOf(segment); });
  ASSERT_TRUE(model.valid);
  ASSERT_EQ(model.nodes.size(), 9u);
  EXPECT_EQ(model.nodes[4].label, "P5");
  EXPECT_EQ(model.nodes[4].parent_id, "p3");
  EXPECT_EQ(model.nodes[4].depth, 4u);
  ASSERT_EQ(model.relations.size(), 4u);
  EXPECT_TRUE(model.relations[0].same_position);
  EXPECT_FALSE(model.relations[1].same_position);
  EXPECT_EQ(model.relations[0].positive_id, "p0");
  EXPECT_EQ(model.relations[0].negative_id, "n0");
  const std::string html = renderVisualReportHtml(model);
  EXPECT_NE(html.find("data-node=\"p4\""), std::string::npos);
  EXPECT_NE(html.find("普通对应"), std::string::npos);
  EXPECT_NE(html.find("同位置"), std::string::npos);
}
