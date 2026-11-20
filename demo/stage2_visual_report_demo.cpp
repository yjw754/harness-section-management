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
  for (const char* label : {"P1", "P2", "P3", "P4", "P5", "P6"}) {
    auto options = member;
    if (conflict && std::string(label) == "P4") {
      options.radius = 1;
      options.auto_resize_enabled = false;
    }
    builder.circularSection(ZeHrnSide::Positive, label, options);
  }
  for (const char* label : {"N1", "N2", "N3"})
    builder.circularSection(ZeHrnSide::Negative, label, member);
  builder.relate("P1", "N1", true).relate("P1", "N2", false)
      .relate("P2", "N2", false);
  // Shared-center siblings are supported; the failure is P5 outside fixed P4.
  if (conflict) builder.relate("P1", "N3", true);
  else builder.relate("P3", "N3", false);
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
hrn::HrnSectionTestCase makeMappedSiblingOverlapCase(bool many_positive) {
  using namespace hrn;
  HrnCircularSectionCaseOptions root;
  root.radius = 20;
  root.auto_resize_enabled = true;
  HrnCircularSectionCaseOptions member;
  member.radius = 8;
  const auto multi = many_positive ? ZeHrnSide::Positive : ZeHrnSide::Negative;
  HrnSectionCaseBuilder builder;
  builder.positionTransform(Matrix4D{}).modelingTolerance(1e-9)
      .segment("p-main").segment("n-main").segment("extra")
      .sideRootCircular(ZeHrnSide::Positive, root)
      .sideRootCircular(ZeHrnSide::Negative, root)
      .circularSection(ZeHrnSide::Positive, "p-main", member)
      .circularSection(ZeHrnSide::Negative, "n-main", member)
      .circularSection(multi, "extra", member)
      .relate("p-main", "n-main", true);
  if (many_positive) builder.relate("extra", "n-main", true);
  else builder.relate("p-main", "extra", true);
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
bool writeCaseMatrixIndex(const std::string& path) {
  std::ofstream file(path.c_str(), std::ios::binary);
  if (!file) return false;
  file << R"HTML(<!doctype html><html lang="zh-CN"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Stage 2 同位置案例审查索引</title><style>
:root{color-scheme:light dark;--bg:#f6f7fb;--card:#fff;--fg:#172033;--muted:#5c667a;--line:#d9deea;--accent:#3159d8}
@media(prefers-color-scheme:dark){:root{--bg:#10131b;--card:#191e2a;--fg:#e8ebf2;--muted:#aab2c2;--line:#343c4d;--accent:#91a8ff}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.55 system-ui,sans-serif}
main{max-width:1180px;margin:auto;padding:32px 20px 48px}h1{margin:0 0 8px;font-size:clamp(1.6rem,4vw,2.4rem)}
.intro{color:var(--muted);max-width:850px}.card{margin-top:24px;background:var(--card);border:1px solid var(--line);border-radius:14px;overflow:auto}
table{width:100%;border-collapse:collapse;min-width:820px}th,td{padding:12px 14px;text-align:left;border-bottom:1px solid var(--line);vertical-align:top}
th{background:color-mix(in srgb,var(--accent) 9%,var(--card))}tr:last-child td{border-bottom:0}a{color:var(--accent);font-weight:650;text-decoration:none}a:hover{text-decoration:underline}
.ok{color:#16834a;font-weight:650}.note{margin-top:18px;padding:14px 16px;border-left:4px solid var(--accent);background:var(--card);border-radius:8px}
</style></head><body><main><h1>Stage 2：0/1/N × 0/1/N 同位置案例</h1>
<p class="intro">九个报告覆盖正负两侧顶层 Section 数量的完整组合。所有可建立的跨侧关系均为“同位置”；空侧组合没有关系。点击案例名称进入可交互的求解报告。</p>
<section class="card" aria-label="案例矩阵"><table><thead><tr><th>正侧 Root</th><th>负侧 Root</th><th>关系类型</th><th>树结构与审查重点</th><th>预期</th><th>报告</th></tr></thead><tbody>
<tr><td>0</td><td>0</td><td>无</td><td>双侧均无成员 Section</td><td class="ok">排布成功</td><td><a href="stage2-roots-0-0.html">0 × 0</a></td></tr>
<tr><td>0</td><td>1</td><td>无</td><td>仅负侧一个顶层 Section</td><td class="ok">排布成功</td><td><a href="stage2-roots-0-1.html">0 × 1</a></td></tr>
<tr><td>1</td><td>0</td><td>无</td><td>仅正侧一个顶层 Section</td><td class="ok">排布成功</td><td><a href="stage2-roots-1-0.html">1 × 0</a></td></tr>
<tr><td>0</td><td>N</td><td>无</td><td>负侧两个顶层 Section，其中一个含子节点</td><td class="ok">排布成功</td><td><a href="stage2-roots-0-n.html">0 × N</a></td></tr>
<tr><td>N</td><td>0</td><td>无</td><td>正侧两个顶层 Section，其中一个含子节点</td><td class="ok">排布成功</td><td><a href="stage2-roots-n-0.html">N × 0</a></td></tr>
<tr><td>1</td><td>1</td><td>一对一</td><td>两侧各一棵树，负侧比正侧深一层</td><td class="ok">排布成功</td><td><a href="stage2-roots-1-1-one-to-one.html">1 × 1</a></td></tr>
<tr><td>1</td><td>N</td><td>一对多</td><td>负侧父子同心嵌套，另有一个无关顶层 Section</td><td class="ok">排布成功</td><td><a href="stage2-roots-1-n-one-to-many.html">1 × N</a></td></tr>
<tr><td>N</td><td>1</td><td>多对一</td><td>正侧父子同心嵌套，另有一个无关顶层 Section</td><td class="ok">排布成功</td><td><a href="stage2-roots-n-1-many-to-one.html">N × 1</a></td></tr>
<tr><td>N</td><td>N</td><td>混合</td><td>两侧各三棵不等深树；一对一、一对多、多对一分组独立</td><td class="ok">排布成功</td><td><a href="stage2-roots-n-n-mixed.html">N × N</a></td></tr>
</tbody></table></section><p class="note">上表的一对多和多对一展示父子嵌套。同位置映射也支持同侧兄弟截面共享圆心并重叠：<a href="stage2-overlap-2-1.html">2 对 1 重叠</a>、<a href="stage2-overlap-1-2.html">1 对 2 重叠</a>。同一映射连通组以最大半径占位，不同组仍满足间距和父边界包含要求。</p>
</main></body></html>)HTML";
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
      !writeCase(directory + "/stage2-roots-n-n-mixed.html", makeMixedManyRootCase(), false) ||
      !writeCase(directory + "/stage2-overlap-2-1.html", makeMappedSiblingOverlapCase(true), false) ||
      !writeCase(directory + "/stage2-overlap-1-2.html", makeMappedSiblingOverlapCase(false), false) ||
      !writeCaseMatrixIndex(directory + "/stage2-case-matrix-index.html"))
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
            << directory + "/stage2-roots-n-n-mixed.html\n"
            << directory + "/stage2-overlap-2-1.html\n"
            << directory + "/stage2-overlap-1-2.html\n"
            << directory + "/stage2-case-matrix-index.html\n";
  return 0;
}
