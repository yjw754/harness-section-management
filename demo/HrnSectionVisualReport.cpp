#include "HrnSectionVisualReport.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>

namespace hrn {
namespace {
std::string escape(const std::string& value) {
  std::string out;
  for (char ch : value) {
    switch (ch) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&#39;"; break;
      default: out += ch;
    }
  }
  return out;
}
std::string number(double value) {
  std::ostringstream out;
  out << std::setprecision(12) << value;
  return out.str();
}
const char* reasonName(HrnSectionFailureReason reason) {
  switch (reason) {
    case HrnSectionFailureReason::None: return "None";
    case HrnSectionFailureReason::InvalidInput: return "InvalidInput";
    case HrnSectionFailureReason::InvalidTopology: return "InvalidTopology";
    case HrnSectionFailureReason::FixedBoundaryInsufficient: return "FixedBoundaryInsufficient";
    case HrnSectionFailureReason::SizeUpdateConstraintConflict: return "SizeUpdateConstraintConflict";
    case HrnSectionFailureReason::SamePositionConstraintConflict: return "SamePositionConstraintConflict";
    case HrnSectionFailureReason::ConstraintInfeasible: return "ConstraintInfeasible";
    case HrnSectionFailureReason::NoSolutionFound: return "NoSolutionFound";
    case HrnSectionFailureReason::IterationBudgetExceeded: return "IterationBudgetExceeded";
    case HrnSectionFailureReason::NonConvergent: return "NonConvergent";
    case HrnSectionFailureReason::GeometryValidationFailed: return "GeometryValidationFailed";
    case HrnSectionFailureReason::InternalError: return "InternalError";
  }
  return "UnknownFailure";
}
std::string nodeId(ZeHrnSide side, size_t index) {
  return std::string(side == ZeHrnSide::Positive ? "p" : "n") + std::to_string(index);
}
const char* stageName(HrnSectionFailureStage stage) {
  switch (stage) {
    case HrnSectionFailureStage::InputValidation: return "InputValidation";
    case HrnSectionFailureStage::TopologyValidation: return "TopologyValidation";
    case HrnSectionFailureStage::LayoutSolve: return "LayoutSolve";
    case HrnSectionFailureStage::ParentSizeUpdate: return "ParentSizeUpdate";
    case HrnSectionFailureStage::GeometryValidation: return "GeometryValidation";
    case HrnSectionFailureStage::Commit: return "Commit";
  }
  return "UnknownStage";
}
const char* conflictName(HrnSectionConflictKind kind) {
  switch (kind) {
    case HrnSectionConflictKind::OutsideParent: return "OutsideParent";
    case HrnSectionConflictKind::Overlap: return "Overlap";
    case HrnSectionConflictKind::ClearanceInsufficient: return "ClearanceInsufficient";
    case HrnSectionConflictKind::FrozenDimensionInsufficient: return "FrozenDimensionInsufficient";
    case HrnSectionConflictKind::SamePositionIncompatible: return "SamePositionIncompatible";
    case HrnSectionConflictKind::InvalidGeometry: return "InvalidGeometry";
  }
  return "UnknownConflict";
}
std::string targetName(const HrnSectionTarget& target, const HrnVisualReportModel& model,
                       const HrnPositionPointInput& input) {
  if (target.kind == HrnSectionTargetKind::PositionPoint) return "Position Point";
  const std::string side = target.side == ZeHrnSide::Positive ? "正侧" : "负侧";
  if (target.kind == HrnSectionTargetKind::SideRootSection) return side + " Root";
  size_t index = 0;
  if (target.segment && input.findSectionIndex(target.side, *target.segment, index)) {
    const std::string id = nodeId(target.side, index);
    for (const auto& node : model.nodes) if (node.id == id) return side + " " + node.label;
  }
  return side + " Section";
}
std::string detail(const HrnVisualNode& node) {
  std::ostringstream out;
  out << node.label << " · " << (node.side == ZeHrnSide::Positive ? "正侧" : "负侧")
      << " · 编号 " << node.id << " · 父节点 " << (node.parent_id.empty() ? "Root" : node.parent_id)
      << " · 输入半径 " << number(node.input_radius) << " · 输入旋转 " << number(node.rotation) << "°";
  if (node.has_geometry)
    out << " · 求解半径 " << number(node.solved_radius)
        << " · 求解旋转 " << number(node.solved_rotation) << "°"
        << " · 2D (" << number(node.center2d.x) << ", " << number(node.center2d.y)
        << ") · 3D (" << number(node.location.x) << ", " << number(node.location.y)
        << ", " << number(node.location.z) << ")";
  return out.str();
}
void renderTree(std::ostringstream& out, const HrnVisualReportModel& model, ZeHrnSide side) {
  out << "<section class=\"panel tree\"><h2>" << (side == ZeHrnSide::Positive ? "正侧层级" : "负侧层级")
      << "</h2><div class=\"tree-list\">";
  std::set<std::string> emitted;
  const std::function<void(const HrnVisualNode&)> emit = [&](const HrnVisualNode& node) {
    if (!emitted.insert(node.id).second) return;
    out << "<button type=\"button\" class=\"tree-node\" data-node=\"" << node.id
        << "\" data-detail=\"" << escape(detail(node)) << "\" style=\"--depth:"
        << node.depth << "\">" << escape(node.label) << " <span class=\"minor\">r="
        << number(node.has_geometry ? node.solved_radius : node.input_radius)
        << "</span></button>";
    for (const auto& child : model.nodes)
      if (child.side == side && child.parent_id == node.id) emit(child);
  };
  for (const auto& node : model.nodes)
    if (node.side == side && node.parent_id.empty()) emit(node);
  for (const auto& node : model.nodes)
    if (node.side == side) emit(node);
  out << "</div></section>";
}
void renderDiagram(std::ostringstream& out, const HrnVisualReportModel& model, ZeHrnSide side) {
  const int side_index = side == ZeHrnSide::Positive ? 0 : 1;
  double extent = model.side_root_radius[side_index];
  for (const auto& node : model.nodes)
    if (node.side == side && node.has_geometry)
      extent = std::max(extent, std::hypot(node.center2d.x, node.center2d.y) + node.solved_radius);
  extent = std::max(1.0, extent) * 1.15;
  const double span = 2 * extent;
  out << "<section class=\"panel diagram\"><div class=\"diagram-head\"><h2>"
      << (side == ZeHrnSide::Positive ? "正侧排布" : "负侧排布")
      << "</h2><div class=\"zoom\"><button type=\"button\" data-action=\"zoom-in\" aria-label=\"放大\">＋</button>"
      << "<button type=\"button\" data-action=\"zoom-out\" aria-label=\"缩小\">−</button>"
      << "<button type=\"button\" data-action=\"fit\">适应</button></div></div>";
  out << "<svg class=\"packing\" role=\"img\" aria-label=\""
      << (side == ZeHrnSide::Positive ? "正侧圆形截面排布" : "负侧圆形截面排布")
      << "\" viewBox=\"" << number(-extent) << " " << number(-extent) << " "
      << number(span) << " " << number(span) << "\" data-base=\""
      << number(-extent) << " " << number(-extent) << " " << number(span) << " "
      << number(span) << "\">";
  out << "<line class=\"axis\" x1=\"" << number(-extent) << "\" y1=\"0\" x2=\""
      << number(extent) << "\" y2=\"0\"/><line class=\"axis\" x1=\"0\" y1=\""
      << number(-extent) << "\" x2=\"0\" y2=\"" << number(extent) << "\"/>";
  if (model.solved)
    out << "<circle class=\"side-root\" cx=\"0\" cy=\"0\" r=\""
        << number(model.side_root_radius[side_index]) << "\"/>";
  std::vector<const HrnVisualNode*> circles;
  for (const auto& node : model.nodes) if (node.side == side && node.has_geometry) circles.push_back(&node);
  std::stable_sort(circles.begin(), circles.end(), [](const HrnVisualNode* a, const HrnVisualNode* b) {
    return a->solved_radius > b->solved_radius;
  });
  for (const auto* node : circles)
    out << "<circle class=\"section-circle\" data-node=\"" << node->id
        << "\" tabindex=\"0\" role=\"button\" aria-label=\"" << escape(detail(*node))
        << "\" cx=\"" << number(node->center2d.x) << "\" cy=\""
        << number(-node->center2d.y) << "\" r=\"" << number(node->solved_radius) << "\"/>";
  out << "</svg><div class=\"axis-label\">X →　Y ↑ · 半径与坐标使用模型单位</div></section>";
}
}  // namespace

HrnVisualReportModel buildVisualReportModel(
    const HrnPositionPointInput& input,
    const std::vector<HrnSegmentSideRelation>& relations,
    const HrnCircularHierarchyResult& solve_result,
    const std::function<std::string(const HrnSegment&)>& labels) {
  HrnVisualReportModel model;
  model.position_center = center(input.positionTransform());
  model.x_axis = {input.positionTransform().m[0][0], input.positionTransform().m[1][0]};
  model.y_axis = {input.positionTransform().m[0][1], input.positionTransform().m[1][1]};
  model.solved = solve_result.status.committed;
  for (int k = 0; k < 2; ++k) {
    const ZeHrnSide side = k == 0 ? ZeHrnSide::Positive : ZeHrnSide::Negative;
    const auto& solved = solve_result.side(side);
    if (model.solved && (solved.segments.size() != input.sectionCount(side) ||
                         solved.sections.size() != input.sectionCount(side) ||
                         solved.centers2d.size() != input.sectionCount(side) ||
                         solved.absolute_rotations.size() != input.sectionCount(side) ||
                         solved.locations.size() != input.sectionCount(side) || !solved.root)) {
      model.error = "求解结果与输入数量不一致";
      return model;
    }
    model.side_root_radius[k] = model.solved ? solved.root->shape().dimensions().primary :
                                           input.sideRootSection(side).shape().dimensions().primary;
    for (size_t i = 0; i < input.sectionCount(side); ++i) {
      const HrnSegment& segment = input.segmentAt(side, i);
      if (model.solved && solved.segments[i] != &segment) {
        model.error = "求解结果与 Segment 顺序不一致";
        return model;
      }
      HrnVisualNode node;
      node.id = nodeId(side, i);
      node.label = labels(segment);
      node.side = side;
      node.input_radius = input.sectionAt(side, i).shape().dimensions().primary;
      node.rotation = input.sectionAt(side, i).rotation();
      size_t parent_index = 0;
      if (segment.parent() && input.findSectionIndex(side, *segment.parent(), parent_index)) {
        node.parent_id = nodeId(side, parent_index);
        size_t walk = parent_index;
        node.depth = 1;
        for (size_t guard = 0; guard < input.sectionCount(side); ++guard) {
          const HrnSegment* parent = input.segmentAt(side, walk).parent();
          if (!parent || !input.findSectionIndex(side, *parent, walk)) break;
          ++node.depth;
        }
      }
      if (model.solved) {
        if (!solved.sections[i]) {
          model.error = "求解结果包含空 Section";
          return model;
        }
        node.has_geometry = true;
        node.solved_radius = solved.sections[i]->shape().dimensions().primary;
        node.solved_rotation = solved.absolute_rotations[i];
        node.center2d = solved.centers2d[i];
        node.location = solved.locations[i];
      }
      model.nodes.push_back(node);
    }
  }
  for (const auto& relation : relations) {
    size_t positive = 0, negative = 0;
    if (!relation.valid() ||
        !input.findSectionIndex(ZeHrnSide::Positive, *relation.positive, positive) ||
        !input.findSectionIndex(ZeHrnSide::Negative, *relation.negative, negative)) {
      model.error = "映射关系引用了缺失的侧内 Segment";
      return model;
    }
    model.relations.push_back({nodeId(ZeHrnSide::Positive, positive),
                               nodeId(ZeHrnSide::Negative, negative), relation.same_position});
  }
  if (!model.solved) {
    model.diagnostics.push_back(reasonName(solve_result.status.primary_reason));
    for (const auto& diagnostic : solve_result.status.diagnostics) {
      std::ostringstream text;
      text << stageName(diagnostic.stage) << " · " << reasonName(diagnostic.reason)
           << " · " << targetName(diagnostic.target, model, input)
           << " · 算法版本 " << diagnostic.algorithm_version
           << " · 迭代 " << diagnostic.iterations_used;
      model.diagnostics.push_back(text.str());
      for (const auto& conflict : diagnostic.conflicts) {
        std::ostringstream details;
        details << conflictName(conflict.kind) << " · " << targetName(conflict.lhs, model, input);
        if (conflict.has_rhs) details << " ↔ " << targetName(conflict.rhs, model, input);
        if (conflict.has_measured_value) details << " · 实测 " << number(conflict.measured_value);
        if (conflict.has_required_value) details << " · 要求 " << number(conflict.required_value);
        model.diagnostics.push_back(details.str());
      }
      if (!diagnostic.adjustments.empty())
        model.diagnostics.push_back("建议调整项 " + std::to_string(diagnostic.adjustments.size()));
    }
  }
  model.valid = true;
  return model;
}

std::string renderVisualReportHtml(const HrnVisualReportModel& model) {
  std::ostringstream out;
  out << "<!doctype html><html lang=\"zh-CN\"><head><meta charset=\"utf-8\">"
      << "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
      << "<title>线束截面可视化</title><style>"
      << ":root{font-family:system-ui,sans-serif;color-scheme:light dark;--bg:Canvas;--fg:CanvasText;--line:color-mix(in srgb,CanvasText 22%,Canvas);--accent:LinkText;--soft:color-mix(in srgb,LinkText 13%,Canvas)}"
      << "*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg)}main{max-width:1500px;margin:auto;padding:24px}h1{font-size:1.5rem;margin:0 0 8px}h2{font-size:1rem;margin:0 0 12px}.summary{opacity:.75;margin-bottom:18px}"
      << ".grid{display:grid;grid-template-columns:minmax(190px,1fr) minmax(220px,.9fr) minmax(190px,1fr);gap:12px}.plots{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-top:12px}.panel{border:1px solid var(--line);border-radius:12px;padding:16px;min-width:0}.tree-list{display:flex;flex-direction:column;gap:3px;max-height:420px;overflow:auto}.tree-node,.map-row{background:transparent;color:inherit;border:0;text-align:left;padding:6px 8px;border-radius:7px;width:100%;font:inherit}.tree-node{padding-left:calc(8px + var(--depth)*17px)}button{cursor:pointer}.tree-node:hover,.map-row:hover,.is-active{background:var(--soft)}.minor{opacity:.6;font-size:.82em}.map-row{display:flex;justify-content:space-between;gap:8px;border-bottom:1px solid var(--line)}.tag{font-size:.75rem;white-space:nowrap}.diagram-head{display:flex;align-items:center;justify-content:space-between;gap:10px}.zoom{display:flex;gap:4px}.zoom button{background:transparent;color:inherit;border:1px solid var(--line);border-radius:6px;padding:3px 8px}.packing{display:block;width:100%;height:min(38vw,420px);min-height:260px;touch-action:none}.axis{stroke:var(--line);stroke-width:.03;vector-effect:non-scaling-stroke}.side-root{fill:none;stroke:var(--line);stroke-width:2;stroke-dasharray:5 4;vector-effect:non-scaling-stroke}.section-circle{fill:var(--soft);stroke:var(--accent);stroke-width:1.5;vector-effect:non-scaling-stroke;cursor:pointer;outline:none}.section-circle.is-active{fill:var(--accent);fill-opacity:.35;stroke-width:3}.section-circle:focus-visible{stroke-dasharray:5 3;stroke-width:3}.axis-label{opacity:.65;font-size:.8rem}.details{margin-top:12px;min-height:58px}.status{font-weight:600}.failure{color:var(--fg);background:var(--soft);padding:9px;border-radius:8px;margin:6px 0}@media(max-width:800px){.grid,.plots{grid-template-columns:1fr}.packing{height:340px}}"
      << "</style></head><body><main><h1>线束截面可视化</h1>";
  if (!model.valid) {
    out << "<p class=\"failure\">报告输入无效：" << escape(model.error) << "</p></main></body></html>";
    return out.str();
  }
  out << "<div class=\"summary\">Position Point 中心 (" << number(model.position_center.x)
      << ", " << number(model.position_center.y) << ", " << number(model.position_center.z)
      << ") · X 轴 (" << number(model.x_axis.x) << ", " << number(model.x_axis.y)
      << ") · Y 轴 (" << number(model.y_axis.x) << ", " << number(model.y_axis.y)
      << ") · <span class=\"status\">" << (model.solved ? "排布成功" : "排布失败")
      << "</span></div><div class=\"grid\">";
  renderTree(out, model, ZeHrnSide::Positive);
  out << "<section class=\"panel mapping\"><h2>两侧映射</h2>";
  for (const auto& relation : model.relations) {
    const HrnVisualNode* positive = nullptr;
    const HrnVisualNode* negative = nullptr;
    for (const auto& node : model.nodes) {
      if (node.id == relation.positive_id) positive = &node;
      if (node.id == relation.negative_id) negative = &node;
    }
    if (!positive || !negative) continue;
    out << "<button type=\"button\" class=\"map-row\" data-positive=\""
        << relation.positive_id << "\" data-negative=\"" << relation.negative_id
        << "\"><span>" << escape(positive->label) << " ↔ " << escape(negative->label)
        << "</span><span class=\"tag\">" << (relation.same_position ? "同位置" : "普通对应")
        << "</span></button>";
  }
  out << "</section>";
  renderTree(out, model, ZeHrnSide::Negative);
  out << "</div>";
  if (model.solved) {
    out << "<div class=\"plots\">";
    renderDiagram(out, model, ZeHrnSide::Positive);
    renderDiagram(out, model, ZeHrnSide::Negative);
    out << "</div>";
  } else {
    out << "<section class=\"panel details\"><h2>求解诊断</h2>";
    for (const auto& diagnostic : model.diagnostics)
      out << "<p class=\"failure\">" << escape(diagnostic) << "</p>";
    out << "</section>";
  }
  out << "<section class=\"panel details\" aria-live=\"polite\"><h2>所选截面</h2>"
      << "<div id=\"section-detail\">点击层级节点、映射或圆形查看参数</div></section>"
      << "</main><script>" R"JS((()=>{const root=document.querySelector('main');const detail=document.getElementById('section-detail');
function select(ids,edge=null){root.querySelectorAll('.is-active').forEach(e=>e.classList.remove('is-active'));
const linked=new Set(ids);root.querySelectorAll('.map-row').forEach(row=>{
if(edge ? row===edge : (ids.includes(row.dataset.positive)||ids.includes(row.dataset.negative))){
linked.add(row.dataset.positive);linked.add(row.dataset.negative);row.classList.add('is-active')}});
linked.forEach(id=>root.querySelectorAll('[data-node="'+id+'"]').forEach(e=>e.classList.add('is-active')));
const selected=ids.map(id=>root.querySelector('.tree-node[data-node="'+id+'"]')).filter(Boolean);
if(selected.length)detail.textContent=selected.map(node=>node.dataset.detail).join('；')}
root.addEventListener('click',event=>{const row=event.target.closest('.map-row');if(row){select([row.dataset.positive,row.dataset.negative],row);return}
const node=event.target.closest('[data-node]');if(node){select([node.dataset.node]);return}
const action=event.target.closest('[data-action]');if(!action)return;const svg=action.closest('.diagram').querySelector('svg');
const base=svg.dataset.base.split(' ').map(Number),current=svg.getAttribute('viewBox').split(' ').map(Number);
if(action.dataset.action==='fit'){svg.setAttribute('viewBox',base.join(' '));return}
const factor=action.dataset.action==='zoom-in' ? 0.7 : 1.3,w=Math.min(base[2]*4,Math.max(base[2]*.08,current[2]*factor));
const h=w*base[3]/base[2],cx=current[0]+current[2]/2,cy=current[1]+current[3]/2;
svg.setAttribute('viewBox',[cx-w/2,cy-h/2,w,h].join(' '))});
root.addEventListener('keydown',event=>{if((event.key==='Enter'||event.key===' ')&&event.target.matches('circle[data-node]')){event.preventDefault();select([event.target.dataset.node])}})
})();)JS" << "</script></body></html>";
  return out.str();
}
}  // namespace hrn
