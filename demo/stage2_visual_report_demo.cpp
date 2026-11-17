#include "HrnSectionVisualReport.h"

#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>

#include "HrnSectionCaseBuilder.h"

namespace {
hrn::HrnSectionTestCase makeCase(bool conflict) {
  using namespace hrn;
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
  builder.relate("P1", "N1", true).relate("P1", "N2", false)
      .relate("P2", "N2", false).relate("P3", "N3", conflict);
  return builder.build();
}
hrn::HrnSectionTestCase makeNestedPairsCase(bool two_pairs) {
  using namespace hrn;
  Matrix4D matrix;
  HrnCircularSectionCaseOptions root;
  root.radius = 20;
  HrnCircularSectionCaseOptions parent;
  parent.radius = 6;
  HrnCircularSectionCaseOptions child;
  child.radius = 1;
  HrnSectionCaseBuilder builder;
  builder.positionTransform(matrix).modelingTolerance(1e-9)
      .segment("p0").segment("p1").segment("p2")
      .segment("n0").segment("n1").segment("n2")
      .parent("p1", "p0").parent("p2", "p0")
      .parent("n1", "n0").parent("n2", "n0")
      .sideRootCircular(ZeHrnSide::Positive, root)
      .sideRootCircular(ZeHrnSide::Negative, root)
      .circularSection(ZeHrnSide::Positive, "p0", parent)
      .circularSection(ZeHrnSide::Positive, "p1", child)
      .circularSection(ZeHrnSide::Positive, "p2", child)
      .circularSection(ZeHrnSide::Negative, "n0", parent)
      .circularSection(ZeHrnSide::Negative, "n1", child)
      .circularSection(ZeHrnSide::Negative, "n2", child)
      .relate("p1", "n1", true);
  if (two_pairs) builder.relate("p2", "n2", true);
  return builder.build();
}
hrn::HrnSectionTestCase makeTwoParentPairsCase() {
  using namespace hrn;
  Matrix4D matrix;
  HrnCircularSectionCaseOptions root;
  root.radius = 20;
  HrnCircularSectionCaseOptions outer;
  outer.radius = 12;
  HrnCircularSectionCaseOptions middle;
  middle.radius = 2;
  HrnCircularSectionCaseOptions leaf;
  leaf.radius = 1;
  HrnSectionCaseBuilder builder;
  builder.positionTransform(matrix).modelingTolerance(1e-9)
      .segment("p0").segment("p1").segment("p2").segment("p3").segment("p4")
      .segment("n0").segment("n1").segment("n2").segment("n3").segment("n4")
      .parent("p0", "p3").parent("p1", "p0").parent("p2", "p0").parent("p4", "p3")
      .parent("n0", "n3").parent("n1", "n3").parent("n2", "n0").parent("n4", "n0")
      .sideRootCircular(ZeHrnSide::Positive, root)
      .sideRootCircular(ZeHrnSide::Negative, root);
  for (const auto side : {ZeHrnSide::Positive, ZeHrnSide::Negative}) {
    const bool positive = side == ZeHrnSide::Positive;
    HrnCircularSectionCaseOptions side_outer = outer;
    if (!positive) side_outer.rotation = 180;
    HrnCircularSectionCaseOptions side_middle = middle;
    if (!positive) side_middle.rotation = 180;
    builder.circularSection(side, positive ? "p0" : "n0", side_middle)
        .circularSection(side, positive ? "p1" : "n1", leaf)
        .circularSection(side, positive ? "p2" : "n2", leaf)
        .circularSection(side, positive ? "p3" : "n3", side_outer)
        .circularSection(side, positive ? "p4" : "n4", leaf);
  }
  builder.relate("p1", "n1", true).relate("p2", "n2", true).relate("p4", "n4", true);
  return builder.build();
}
hrn::HrnSectionTestCase makeRootCountCase(int positive_roots, int negative_roots) {
  using namespace hrn;
  Matrix4D matrix;
  HrnCircularSectionCaseOptions side_root;
  side_root.radius = 20;
  HrnCircularSectionCaseOptions outer;
  outer.radius = 8;
  HrnCircularSectionCaseOptions nested;
  nested.radius = 3;
  HrnCircularSectionCaseOptions other;
  other.radius = 4;
  HrnSectionCaseBuilder builder;
  builder.positionTransform(matrix).modelingTolerance(1e-9)
      .sideRootCircular(ZeHrnSide::Positive, side_root)
      .sideRootCircular(ZeHrnSide::Negative, side_root);
  if (positive_roots == 1) {
    builder.segment("zero-positive-root")
        .circularSection(ZeHrnSide::Positive, "zero-positive-root", outer);
  } else if (positive_roots > 1) {
    builder.segment("positive-root-a").segment("positive-root-a-child")
        .segment("positive-root-b")
        .parent("positive-root-a-child", "positive-root-a")
        .circularSection(ZeHrnSide::Positive, "positive-root-a", outer)
        .circularSection(ZeHrnSide::Positive, "positive-root-a-child", nested)
        .circularSection(ZeHrnSide::Positive, "positive-root-b", other);
  }
  if (negative_roots == 1) {
    builder.segment("zero-negative-root")
        .circularSection(ZeHrnSide::Negative, "zero-negative-root", outer);
  } else if (negative_roots > 1) {
    builder.segment("negative-root-a").segment("negative-root-a-child")
        .segment("negative-root-b")
        .parent("negative-root-a-child", "negative-root-a")
        .circularSection(ZeHrnSide::Negative, "negative-root-a", outer)
        .circularSection(ZeHrnSide::Negative, "negative-root-a-child", nested)
        .circularSection(ZeHrnSide::Negative, "negative-root-b", other);
  }
  return builder.build();
}
hrn::HrnSectionTestCase makeOneToOneRootCase() {
  using namespace hrn;
  Matrix4D matrix;
  HrnCircularSectionCaseOptions side_root;
  side_root.radius = 20;
  HrnCircularSectionCaseOptions positive_outer;
  positive_outer.radius = 8;
  HrnCircularSectionCaseOptions positive_child;
  positive_child.radius = 3;
  HrnCircularSectionCaseOptions negative_outer;
  negative_outer.radius = 9;
  HrnCircularSectionCaseOptions negative_child;
  negative_child.radius = 4;
  HrnCircularSectionCaseOptions negative_leaf;
  negative_leaf.radius = 1;
  HrnSectionCaseBuilder builder;
  builder.positionTransform(matrix).modelingTolerance(1e-9)
      .segment("one-positive-root").segment("positive-nested-child")
      .segment("one-negative-root").segment("negative-nested-child")
      .segment("negative-deep-leaf")
      .parent("positive-nested-child", "one-positive-root")
      .parent("negative-nested-child", "one-negative-root")
      .parent("negative-deep-leaf", "negative-nested-child")
      .sideRootCircular(ZeHrnSide::Positive, side_root)
      .sideRootCircular(ZeHrnSide::Negative, side_root)
      .circularSection(ZeHrnSide::Positive, "one-positive-root", positive_outer)
      .circularSection(ZeHrnSide::Positive, "positive-nested-child", positive_child)
      .circularSection(ZeHrnSide::Negative, "one-negative-root", negative_outer)
      .circularSection(ZeHrnSide::Negative, "negative-nested-child", negative_child)
      .circularSection(ZeHrnSide::Negative, "negative-deep-leaf", negative_leaf)
      .relate("positive-nested-child", "negative-nested-child", true);
  return builder.build();
}
hrn::HrnSectionTestCase makeOneToManyCase() {
  using namespace hrn;
  Matrix4D matrix;
  HrnCircularSectionCaseOptions side_root;
  side_root.radius = 20;
  HrnCircularSectionCaseOptions mapped;
  mapped.radius = 8;
  HrnCircularSectionCaseOptions nested;
  nested.radius = 3;
  HrnCircularSectionCaseOptions unrelated;
  unrelated.radius = 4;
  HrnSectionCaseBuilder builder;
  builder.positionTransform(matrix).modelingTolerance(1e-9)
      .segment("otm-positive-mapped")
      .segment("otm-negative-parent").segment("otm-negative-child")
      .segment("otm-negative-extra-root")
      .parent("otm-negative-child", "otm-negative-parent")
      .sideRootCircular(ZeHrnSide::Positive, side_root)
      .sideRootCircular(ZeHrnSide::Negative, side_root)
      .circularSection(ZeHrnSide::Positive, "otm-positive-mapped", mapped)
      .circularSection(ZeHrnSide::Negative, "otm-negative-parent", mapped)
      .circularSection(ZeHrnSide::Negative, "otm-negative-child", nested)
      .circularSection(ZeHrnSide::Negative, "otm-negative-extra-root", unrelated)
      .relate("otm-positive-mapped", "otm-negative-parent", true)
      .relate("otm-positive-mapped", "otm-negative-child", true);
  return builder.build();
}
hrn::HrnSectionTestCase makeManyToOneCase() {
  using namespace hrn;
  Matrix4D matrix;
  HrnCircularSectionCaseOptions side_root;
  side_root.radius = 20;
  HrnCircularSectionCaseOptions mapped;
  mapped.radius = 8;
  HrnCircularSectionCaseOptions nested;
  nested.radius = 3;
  HrnCircularSectionCaseOptions unrelated;
  unrelated.radius = 4;
  HrnSectionCaseBuilder builder;
  builder.positionTransform(matrix).modelingTolerance(1e-9)
      .segment("mto-positive-parent").segment("mto-positive-child")
      .segment("mto-positive-extra-root")
      .segment("mto-negative-mapped")
      .parent("mto-positive-child", "mto-positive-parent")
      .sideRootCircular(ZeHrnSide::Positive, side_root)
      .sideRootCircular(ZeHrnSide::Negative, side_root)
      .circularSection(ZeHrnSide::Positive, "mto-positive-parent", mapped)
      .circularSection(ZeHrnSide::Positive, "mto-positive-child", nested)
      .circularSection(ZeHrnSide::Positive, "mto-positive-extra-root", unrelated)
      .circularSection(ZeHrnSide::Negative, "mto-negative-mapped", mapped)
      .relate("mto-positive-parent", "mto-negative-mapped", true)
      .relate("mto-positive-child", "mto-negative-mapped", true);
  return builder.build();
}
hrn::HrnSectionTestCase makeMixedManyRootCase() {
  using namespace hrn;
  Matrix4D matrix;
  HrnCircularSectionCaseOptions side_root;
  side_root.radius = 30;
  HrnCircularSectionCaseOptions large;
  large.radius = 8;
  HrnCircularSectionCaseOptions medium;
  medium.radius = 6;
  HrnCircularSectionCaseOptions nested;
  nested.radius = 3;
  HrnCircularSectionCaseOptions leaf;
  leaf.radius = 1;
  HrnSectionCaseBuilder builder;
  builder.positionTransform(matrix).modelingTolerance(1e-9)
      .segment("mix-p-one").segment("mix-p-one-child").segment("mix-p-one-leaf")
      .segment("mix-p-otm")
      .segment("mix-p-mto-parent").segment("mix-p-mto-child")
      .segment("mix-n-one")
      .segment("mix-n-otm-parent").segment("mix-n-otm-child").segment("mix-n-otm-leaf")
      .segment("mix-n-mto")
      .parent("mix-p-one-child", "mix-p-one")
      .parent("mix-p-one-leaf", "mix-p-one-child")
      .parent("mix-p-mto-child", "mix-p-mto-parent")
      .parent("mix-n-otm-child", "mix-n-otm-parent")
      .parent("mix-n-otm-leaf", "mix-n-otm-child")
      .sideRootCircular(ZeHrnSide::Positive, side_root)
      .sideRootCircular(ZeHrnSide::Negative, side_root)
      .circularSection(ZeHrnSide::Positive, "mix-p-one", medium)
      .circularSection(ZeHrnSide::Positive, "mix-p-one-child", nested)
      .circularSection(ZeHrnSide::Positive, "mix-p-one-leaf", leaf)
      .circularSection(ZeHrnSide::Positive, "mix-p-otm", large)
      .circularSection(ZeHrnSide::Positive, "mix-p-mto-parent", large)
      .circularSection(ZeHrnSide::Positive, "mix-p-mto-child", nested)
      .circularSection(ZeHrnSide::Negative, "mix-n-one", medium)
      .circularSection(ZeHrnSide::Negative, "mix-n-otm-parent", large)
      .circularSection(ZeHrnSide::Negative, "mix-n-otm-child", nested)
      .circularSection(ZeHrnSide::Negative, "mix-n-otm-leaf", leaf)
      .circularSection(ZeHrnSide::Negative, "mix-n-mto", large)
      .relate("mix-p-one", "mix-n-one", true)
      .relate("mix-p-otm", "mix-n-otm-parent", true)
      .relate("mix-p-otm", "mix-n-otm-child", true)
      .relate("mix-p-mto-parent", "mix-n-mto", true)
      .relate("mix-p-mto-child", "mix-n-mto", true);
  return builder.build();
}
bool writeCase(const std::string& path, hrn::HrnSectionTestCase candidate, bool expect_conflict) {
  if (!candidate.succeeded) {
    std::cerr << candidate.error << '\n';
    return false;
  }
  hrn::HrnPositionPointSolver solver;
  auto solved = solver.solveCircular(*candidate.input, candidate.relations);
  if (solved.status.committed == expect_conflict) {
    std::cerr << "unexpected solve status for " << path << '\n';
    return false;
  }
  auto model = hrn::buildVisualReportModel(*candidate.input, candidate.relations, solved,
      [&](const hrn::HrnSegment& segment) { return candidate.graph.labelOf(segment); });
  if (!model.valid) {
    std::cerr << model.error << '\n';
    return false;
  }
  std::ofstream file(path.c_str(), std::ios::binary);
  if (!file) return false;
  file << hrn::renderVisualReportHtml(model);
  return static_cast<bool>(file);
}
}  // namespace
int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: stage2_visual_report_demo <output-directory>\n";
    return 2;
  }
  const std::string directory = argv[1];
  if (mkdir(directory.c_str(), 0755) != 0) {
    struct stat info;
    if (stat(directory.c_str(), &info) != 0 || !S_ISDIR(info.st_mode)) {
      std::cerr << "cannot create output directory: " << directory << '\n';
      return 2;
    }
  }
  if (!writeCase(directory + "/stage2-solved.html", makeCase(false), false) ||
      !writeCase(directory + "/stage1-mapping-conflict.html", makeCase(true), true) ||
      !writeCase(directory + "/stage2-nested-pairs.html", makeNestedPairsCase(true), false) ||
      !writeCase(directory + "/stage2-single-mapped-pair.html", makeNestedPairsCase(false), false) ||
      !writeCase(directory + "/stage2-two-parent-pairs.html", makeTwoParentPairsCase(), false) ||
      !writeCase(directory + "/stage2-roots-0-0.html", makeRootCountCase(0, 0), false) ||
      !writeCase(directory + "/stage2-roots-0-1.html", makeRootCountCase(0, 1), false) ||
      !writeCase(directory + "/stage2-roots-1-0.html", makeRootCountCase(1, 0), false) ||
      !writeCase(directory + "/stage2-roots-0-n.html", makeRootCountCase(0, 2), false) ||
      !writeCase(directory + "/stage2-roots-n-0.html", makeRootCountCase(2, 0), false) ||
      !writeCase(directory + "/stage2-roots-1-1-one-to-one.html", makeOneToOneRootCase(), false) ||
      !writeCase(directory + "/stage2-roots-1-n-one-to-many.html", makeOneToManyCase(), false) ||
      !writeCase(directory + "/stage2-roots-n-1-many-to-one.html", makeManyToOneCase(), false) ||
      !writeCase(directory + "/stage2-roots-n-n-mixed.html", makeMixedManyRootCase(), false))
    return 1;
  std::cout << directory + "/stage2-solved.html\n"
            << directory + "/stage1-mapping-conflict.html\n"
            << directory + "/stage2-nested-pairs.html\n"
            << directory + "/stage2-single-mapped-pair.html\n"
            << directory + "/stage2-two-parent-pairs.html\n"
            << directory + "/stage2-roots-0-0.html\n"
            << directory + "/stage2-roots-0-1.html\n"
            << directory + "/stage2-roots-1-0.html\n"
            << directory + "/stage2-roots-0-n.html\n"
            << directory + "/stage2-roots-n-0.html\n"
            << directory + "/stage2-roots-1-1-one-to-one.html\n"
            << directory + "/stage2-roots-1-n-one-to-many.html\n"
            << directory + "/stage2-roots-n-1-many-to-one.html\n"
            << directory + "/stage2-roots-n-n-mixed.html\n";
  return 0;
}
