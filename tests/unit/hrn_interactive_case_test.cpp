#include <gtest/gtest.h>

#include <algorithm>
#include <functional>
#include <limits>

#include "HrnInteractiveCase.h"
#include "HrnSectionCaseBuilder.h"
#include "HrnSectionVisualReport.h"

using namespace hrn;

namespace {
constexpr double kTolerance = 1e-9;

HrnInteractiveSectionDefinition member(const std::string& id, const std::string& side,
                                        double radius, const std::string& parent = "") {
  return {id, id, side, parent, {{"circular", radius}, 0, 0, false}};
}

HrnInteractiveCaseDefinition emptyCase() {
  HrnInteractiveCaseDefinition draft;
  draft.roots[0].shape = {"circular", 20};
  draft.roots[1].shape = {"circular", 20};
  return draft;
}

HrnInteractiveCaseDefinition asymmetricCase() {
  auto draft = emptyCase();
  draft.sections = {member("p-main", "positive", 8), member("p-child", "positive", 3, "p-main"),
                    member("n-main", "negative", 9), member("n-child", "negative", 4, "n-main"),
                    member("n-leaf", "negative", 1, "n-child")};
  draft.relations = {{"p-child", "n-child", true}};
  return draft;
}

const HrnInteractiveNodeResult* findNode(const HrnInteractiveCaseResult& result,
                                         const std::string& id) {
  auto node = std::find_if(result.nodes.begin(), result.nodes.end(),
                           [&](const HrnInteractiveNodeResult& n) { return n.id == id; });
  return node == result.nodes.end() ? nullptr : &*node;
}

HrnCircularSectionCaseOptions nativeCircle(double radius) {
  HrnCircularSectionCaseOptions options;
  options.radius = radius;
  return options;
}

HrnSectionTestCase buildNativeAsymmetric(double positive_parent_radius) {
  using Side = ZeHrnSide;
  HrnSectionCaseBuilder builder;
  return builder.positionTransform(Matrix4D{}).modelingTolerance(kTolerance)
      .segment("p-main").segment("p-child").segment("n-main").segment("n-child").segment("n-leaf")
      .parent("p-child", "p-main").parent("n-child", "n-main").parent("n-leaf", "n-child")
      .sideRootCircular(Side::Positive, nativeCircle(20))
      .sideRootCircular(Side::Negative, nativeCircle(20))
      .circularSection(Side::Positive, "p-main", nativeCircle(positive_parent_radius))
      .circularSection(Side::Positive, "p-child", nativeCircle(3))
      .circularSection(Side::Negative, "n-main", nativeCircle(9))
      .circularSection(Side::Negative, "n-child", nativeCircle(4))
      .circularSection(Side::Negative, "n-leaf", nativeCircle(1))
      .relate("p-child", "n-child", true).build();
}
}  // namespace

TEST(HrnInteractiveCase, RotationRecentersLinkedParentToFitFixedRoot) {
  auto draft = emptyCase();
  draft.roots[1].shape.radius = 10;
  draft.sections = {member("p-child", "positive", 1), member("p-spare", "positive", 1),
                    member("n-parent", "negative", 1), member("n-spare", "negative", 1),
                    member("n-child", "negative", 1, "n-parent")};
  draft.sections[2].options.auto_resize = true;
  draft.relations = {{"p-child", "n-child", true}};
  HrnInteractiveRotationBaseline baseline;
  baseline.nodes = {{"p-child", {{-4, 0}, 0}}, {"p-spare", {{-4, 6}, 0}},
                    {"n-parent", {{-4, 0}, 0}}, {"n-spare", {{5, 7}, 0}},
                    {"n-child", {{-4, 0}, 0}}};
  ASSERT_TRUE(solveInteractiveCase(draft, &baseline).solved);
  draft.roots[0].rotation = 180;

  const auto rotated = solveInteractiveCase(draft, &baseline);
  ASSERT_TRUE(rotated.solved);
  for (const auto* id : {"p-child", "n-child", "n-parent"}) {
    ASSERT_NE(findNode(rotated, id), nullptr);
    EXPECT_NEAR(findNode(rotated, id)->center.x, 4, kTolerance);
    EXPECT_NEAR(findNode(rotated, id)->center.y, 0, kTolerance);
  }
  const auto* spare = findNode(rotated, "n-spare");
  ASSERT_NE(spare, nullptr);
  EXPECT_DOUBLE_EQ(spare->center.x, 5);
  EXPECT_DOUBLE_EQ(spare->center.y, 7);
}

TEST(HrnInteractiveCase, NestedRotationRepairUsesResizedAncestor) {
  auto draft = emptyCase();
  draft.sections = {member("p-child", "positive", 1), member("p-other", "positive", 1),
                    member("p-center", "positive", 1), member("n-grand", "negative", 1),
                    member("n-spare", "negative", 1), member("n-parent", "negative", 1, "n-grand"),
                    member("n-other", "negative", 1, "n-grand"),
                    member("n-child", "negative", 1, "n-parent")};
  draft.sections[3].options.auto_resize = true;
  draft.sections[5].options.auto_resize = true;
  draft.relations = {{"p-child", "n-child", true}, {"p-other", "n-other", true},
                     {"p-center", "n-grand", true}};
  HrnInteractiveRotationBaseline baseline;
  baseline.nodes = {{"p-child", {{-4, 0}, 0}}, {"p-other", {{-4, 3}, 0}}, {"p-center", {{0, 0}, 0}},
                    {"n-grand", {{0, 0}, 0}}, {"n-spare", {{0, 8}, 0}}, {"n-parent", {{-4, 0}, 0}},
                    {"n-other", {{-4, 3}, 0}}, {"n-child", {{-4, 0}, 0}}};
  ASSERT_TRUE(solveInteractiveCase(draft, &baseline).solved);
  draft.roots[0].rotation = 180;

  const auto rotated = solveInteractiveCase(draft, &baseline);
  ASSERT_TRUE(rotated.solved);
  const auto* parent = findNode(rotated, "n-parent");
  const auto* grand = findNode(rotated, "n-grand");
  const auto* spare = findNode(rotated, "n-spare");
  ASSERT_TRUE(parent && grand && spare);
  EXPECT_NEAR(parent->center.x, 4, kTolerance);
  EXPECT_NEAR(parent->solved_shape.radius, 1, kTolerance);
  EXPECT_NEAR(grand->solved_shape.radius, 6, kTolerance);
  EXPECT_DOUBLE_EQ(spare->center.x, 0);
  EXPECT_DOUBLE_EQ(spare->center.y, 8);
}

TEST(HrnInteractiveCase, SideRootRotationKeepsUnblockedFollowerSibling) {
  auto draft = emptyCase();
  draft.sections = {member("p-main", "positive", 1), member("p-spare", "positive", 1),
                    member("n-main", "negative", 1), member("n-spare", "negative", 1)};
  draft.relations = {{"p-main", "n-main", true}};
  HrnInteractiveRotationBaseline baseline;
  baseline.nodes = {{"p-main", {{-4, 0}, 0}}, {"p-spare", {{4, 0}, 0}},
                    {"n-main", {{-4, 0}, 0}}, {"n-spare", {{0, 8}, 0}}};
  ASSERT_TRUE(solveInteractiveCase(draft, &baseline).solved);
  draft.roots[0].rotation = 90;

  const auto rotated = solveInteractiveCase(draft, &baseline);
  ASSERT_TRUE(rotated.solved);
  const auto* linked = findNode(rotated, "n-main");
  const auto* spare = findNode(rotated, "n-spare");
  ASSERT_TRUE(linked && spare);
  EXPECT_NEAR(linked->center.x, 0, kTolerance);
  EXPECT_NEAR(linked->center.y, -4, kTolerance);
  EXPECT_DOUBLE_EQ(spare->center.x, 0);
  EXPECT_DOUBLE_EQ(spare->center.y, 8);
}

TEST(HrnInteractiveCase, InfeasibleLocalEditUsesRigidRotationWithoutAnExplicitDelta) {
  auto draft = emptyCase();
  draft.sections = {member("p-main", "positive", 8), member("n-main", "negative", 8),
                    member("p-left", "positive", 3, "p-main"), member("p-right", "positive", 3, "p-main"),
                    member("n-left", "negative", 3, "n-main"), member("n-right", "negative", 3, "n-main")};
  draft.relations = {{"p-left", "n-left", true}};
  HrnInteractiveRotationBaseline baseline;
  baseline.prefer_negative = true;
  baseline.nodes = {{"p-main", {{0, 0}, 0}}, {"n-main", {{0, 0}, 0}},
                    {"p-left", {{-3, 0}, 0}}, {"p-right", {{3, 0}, 0}},
                    {"n-left", {{-3, 0}, 0}}, {"n-right", {{3, 0}, 0}}};
  ASSERT_TRUE(solveInteractiveCase(draft, &baseline).solved);
  draft.sections[1].options.rotation = 180;

  const auto rotated = solveInteractiveCase(draft, &baseline);
  ASSERT_TRUE(rotated.solved);
  ASSERT_TRUE(rotated.used_global_rotation);
  for (const auto& node : rotated.nodes) {
    const auto previous = baseline.nodes.at(node.id).center;
    EXPECT_NEAR(node.center.x, -previous.x, kTolerance);
    EXPECT_NEAR(node.center.y, -previous.y, kTolerance);
    EXPECT_NEAR(node.solved_rotation, 180, kTolerance);
  }
}

TEST(HrnInteractiveCase, SolvedPlacementIsBoundToItsNonRotationConstraints) {
  auto draft = asymmetricCase();
  const auto initial = solveInteractiveCase(draft);
  ASSERT_TRUE(initial.solved);
  ASSERT_TRUE(initial.placement);
  HrnInteractiveRotationBaseline baseline;
  baseline.placement = initial.placement;
  draft.sections[0].options.rotation = 45;
  EXPECT_TRUE(solveInteractiveCase(draft, &baseline).solved);

  const std::vector<std::function<void(HrnInteractiveCaseDefinition&)>> changes{
      [](auto& edited) { edited.sections[0].options.shape.radius += 1; },
      [](auto& edited) { edited.sections[0].options.spacing += 0.1; },
      [](auto& edited) { edited.sections[1].parent_id.clear(); },
      [](auto& edited) { edited.relations[0].same_position = false; },
      [](auto& edited) { edited.position_transform.m[0][3] = 1; },
  };
  for (const auto& change : changes) {
    auto edited = draft;
    change(edited);
    const auto rejected = solveInteractiveCase(edited, &baseline);
    EXPECT_FALSE(rejected.solved);
    EXPECT_FALSE(rejected.placement);
    EXPECT_NE(std::find(rejected.diagnostics.begin(), rejected.diagnostics.end(), "InvalidInput"),
              rejected.diagnostics.end());
  }
}

TEST(HrnInteractiveCase, SolvedPlacementDoesNotFollowReassignedSectionIDs) {
  auto draft = emptyCase();
  draft.sections = {member("p-first", "positive", 4), member("p-second", "positive", 4),
                    member("n-first", "negative", 4), member("n-second", "negative", 4)};
  draft.relations = {{"p-first", "n-first", true}};
  const auto initial = solveInteractiveCase(draft);
  ASSERT_TRUE(initial.solved);
  HrnInteractiveRotationBaseline baseline;
  baseline.placement = initial.placement;
  std::swap(draft.sections[0], draft.sections[1]);
  draft.relations[0].positive_id = "p-second";
  EXPECT_FALSE(solveInteractiveCase(draft, &baseline).solved);
}

TEST(HrnInteractiveCase, RootCountsAndCardinalities) {
  for (int p = 0; p < 3; ++p) {
    for (int n = 0; n < 3; ++n) {
      SCOPED_TRACE(::testing::Message() << p << " × " << n);
      auto draft = emptyCase();
      for (int i = 0; i < p; ++i)
        draft.sections.push_back(member("positive-" + std::to_string(i), "positive", 3));
      for (int i = 0; i < n; ++i)
        draft.sections.push_back(member("negative-" + std::to_string(i), "negative", 3));
      const auto result = solveInteractiveCase(draft);
      ASSERT_TRUE(result.valid);
      ASSERT_TRUE(result.solved);
      EXPECT_EQ(result.nodes.size(), static_cast<size_t>(p + n));
      EXPECT_TRUE(result.relations.empty());
      EXPECT_DOUBLE_EQ(result.roots[0].solved_shape.radius, 20);
      EXPECT_DOUBLE_EQ(result.roots[1].solved_shape.radius, 20);
      for (const auto& node : result.nodes) EXPECT_TRUE(node.parent_id.empty());
    }
  }
  for (bool nested_positive : {true, false}) {
    auto draft = emptyCase();
    const std::string nested = nested_positive ? "positive" : "negative";
    const std::string single = nested_positive ? "negative" : "positive";
    draft.sections = {member("other", single, 8), member("parent", nested, 8),
                      member("child", nested, 3, "parent"), member("unrelated", nested, 4)};
    draft.relations = nested_positive ?
        std::vector<HrnInteractiveRelation>{{"parent", "other", true}, {"child", "other", true}} :
        std::vector<HrnInteractiveRelation>{{"other", "parent", true}, {"other", "child", true}};
    const auto result = solveInteractiveCase(draft);
    ASSERT_TRUE(result.solved);
    ASSERT_EQ(result.relations.size(), 2u);
    for (const auto& link : result.relations) {
      EXPECT_TRUE(link.same_position);
      auto p = findNode(result, link.positive_id);
      auto n = findNode(result, link.negative_id);
      ASSERT_TRUE(p && n && p->has_geometry && n->has_geometry);
      EXPECT_NEAR(p->center.x, n->center.x, kTolerance);
      EXPECT_NEAR(p->center.y, n->center.y, kTolerance);
    }
  }
}

TEST(HrnInteractiveCase, MatchesNativeVersionOne) {
  const std::vector<std::string> ids{"p-main", "p-child", "n-main", "n-child", "n-leaf"};
  const std::vector<std::string> parents{"", "p-main", "", "n-main", "n-child"};
  for (double parent_radius : {8.0, 2.0}) {
    auto draft = asymmetricCase();
    draft.sections[0].options.shape.radius = parent_radius;
    const auto result = solveInteractiveCase(draft);
    auto native = buildNativeAsymmetric(parent_radius);
    ASSERT_TRUE(native.succeeded);
    const auto solved = HrnPositionPointSolver().solveCircular(*native.input, native.relations);
    const auto report = buildVisualReportModel(*native.input, native.relations, solved,
        [&](const HrnSegment& segment) { return native.graph.labelOf(segment); });
    ASSERT_TRUE(result.valid && report.valid);
    EXPECT_EQ(result.solved, parent_radius == 8);
    EXPECT_EQ(result.solved, report.solved);
    EXPECT_EQ(result.diagnostics, report.diagnostics);
    ASSERT_EQ(result.nodes.size(), ids.size());
    for (size_t side = 0; side < result.roots.size(); ++side) {
      const auto& root = result.roots[side];
      EXPECT_NEAR(root.input_shape.radius, 20, kTolerance);
      if (result.solved) EXPECT_NEAR(root.solved_shape.radius, report.side_root_radius[side], kTolerance);
      else EXPECT_TRUE(root.solved_shape.kind.empty());
    }
    for (size_t i = 0; i < ids.size(); ++i) {
      const auto* node = findNode(result, ids[i]);
      ASSERT_NE(node, nullptr);
      EXPECT_EQ(node->parent_id, parents[i]);
      EXPECT_NEAR(node->input_shape.radius, report.nodes[i].input_radius, kTolerance);
      EXPECT_EQ(node->has_geometry, result.solved);
      if (node->has_geometry) {
        EXPECT_NEAR(node->solved_shape.radius, report.nodes[i].solved_radius, kTolerance);
        EXPECT_NEAR(node->center.x, report.nodes[i].center2d.x, kTolerance);
        EXPECT_NEAR(node->center.y, report.nodes[i].center2d.y, kTolerance);
      } else {
        EXPECT_TRUE(node->solved_shape.kind.empty());
      }
    }
  }
}

TEST(HrnInteractiveCase, StableIdsAreNotLabelsOrIndices) {
  auto draft = asymmetricCase();
  std::reverse(draft.sections.begin(), draft.sections.end());
  for (auto& section : draft.sections) section.label = "同名";
  const auto result = solveInteractiveCase(draft);
  ASSERT_TRUE(result.solved);
  auto p = findNode(result, "p-child"), n = findNode(result, "n-leaf");
  ASSERT_TRUE(p && n);
  EXPECT_EQ(p->parent_id, "p-main");
  EXPECT_EQ(n->parent_id, "n-child");
  EXPECT_EQ(n->depth, 2u);
  EXPECT_EQ(n->label, "同名");
  ASSERT_EQ(result.relations.size(), 1u);
  EXPECT_EQ(result.relations[0].positive_id, "p-child");
  EXPECT_EQ(result.relations[0].negative_id, "n-child");
}

TEST(HrnInteractiveCase, RejectsBadDraft) {
  using Case = HrnInteractiveCaseDefinition;
  constexpr double infinity = std::numeric_limits<double>::infinity();
  constexpr double nan = std::numeric_limits<double>::quiet_NaN();
  const std::vector<std::pair<std::string, std::function<void(Case&)>>> cases{
      {"schemaVersion", [](Case& c) { c.schema_version = 2; }},
      {"modelingTolerance", [](Case& c) { c.modeling_tolerance = 0; }},
      {"modelingTolerance", [](Case& c) { c.modeling_tolerance = 1e-12; }},
      {"positionTransform", [](Case& c) { c.position_transform.m[0][0] = 2; }},
      {"positionTransform", [=](Case& c) { c.position_transform.m[0][3] = infinity; }},
      {"roots.positive.shape.kind", [](Case& c) { c.roots[0].shape.kind.clear(); }},
      {"roots.negative.shape.kind", [](Case& c) { c.roots[1].shape.kind = "elliptical"; }},
      {"roots.positive.shape.radius", [](Case& c) { c.roots[0].shape.radius = kTolerance; }},
      {"sections[0].shape.kind", [](Case& c) { c.sections[0].options.shape.kind = "unknown"; }},
      {"sections[0].shape.kind", [](Case& c) { c.sections[0].options.shape.kind = "rectangular"; }},
      {"sections[1].rotation", [=](Case& c) { c.sections[1].options.rotation = nan; }},
      {"sections[1].spacing", [=](Case& c) { c.sections[1].options.spacing = infinity; }},
      {"sections[1].spacing", [](Case& c) { c.sections[1].options.spacing = -1; }},
      {"sections[1].id", [](Case& c) { c.sections[1].id.clear(); }},
      {"sections[1].id", [](Case& c) { c.sections[1].id = "p-main"; }},
      {"sections[1].side", [](Case& c) { c.sections[1].side = "other"; }},
      {"sections[1].parentId", [](Case& c) { c.sections[1].parent_id = "missing"; }},
      {"sections[1].parentId", [](Case& c) { c.sections[1].parent_id = "p-child"; }},
      {"sections[1].parentId", [](Case& c) { c.sections[1].parent_id = "n-main"; }},
      {"sections[0].parentId", [](Case& c) { c.sections[0].parent_id = "p-child"; }},
      {"relations[0].positiveId", [](Case& c) { c.relations[0].positive_id = "n-child"; }},
      {"relations[0].negativeId", [](Case& c) { c.relations[0].negative_id = "missing"; }},
      {"relations[1]", [](Case& c) { c.relations.push_back(c.relations[0]); }},
  };
  for (const auto& item : cases) {
    SCOPED_TRACE(item.first);
    auto draft = asymmetricCase();
    item.second(draft);
    const auto result = solveInteractiveCase(draft);
    EXPECT_FALSE(result.valid || result.solved);
    EXPECT_TRUE(result.nodes.empty());
    EXPECT_TRUE(result.roots[0].solved_shape.kind.empty());
    EXPECT_TRUE(std::any_of(result.errors.begin(), result.errors.end(),
        [&](const HrnInteractiveFieldError& error) { return error.path == item.first; }));
  }
}
