#include "HrnSectionCaseFormatter.h"

#include <functional>
#include <sstream>
#include <vector>

namespace hrn {
namespace {
void formatSide(std::ostringstream& out, const HrnSectionTestCase& c, ZeHrnSide side,
                const char* name) {
  const HrnPositionPointInput& input = *c.input;
  const size_t count = input.sectionCount(side);
  std::vector<std::vector<size_t>> children(count);
  std::vector<size_t> roots;
  for (size_t i = 0; i < count; ++i) {
    HrnSegment* parent = input.segmentAt(side, i).parent();
    size_t parent_index = 0;
    if (parent && input.findSectionIndex(side, *parent, parent_index))
      children[parent_index].push_back(i);
    else
      roots.push_back(i);
  }

  out << name << "（" << count << " 个 Segment；" << roots.size() << " 个根）\n";
  const HrnSectionData& root = input.sideRootSection(side);
  out << "  侧级 Root Section：圆形 r=" << root.shape().dimensions().primary
      << "；自动尺寸=" << (root.autoResizeEnabled() ? "开" : "关") << "；"
      << (roots.size() > 1 ? "参与多根排布" : "保留，当前不参与排布") << '\n';

  std::function<void(size_t, const std::string&, bool)> append =
      [&](size_t index, const std::string& prefix, bool last) {
        const HrnSectionData& section = input.sectionAt(side, index);
        out << prefix << (last ? "└─ " : "├─ ") << c.graph.labelOf(input.segmentAt(side, index))
            << "：圆形 r=" << section.shape().dimensions().primary
            << "，旋转=" << section.rotation() << "°\n";
        const std::string child_prefix = prefix + (last ? "   " : "│  ");
        for (size_t i = 0; i < children[index].size(); ++i)
          append(children[index][i], child_prefix, i + 1 == children[index].size());
      };
  for (size_t i = 0; i < roots.size(); ++i) append(roots[i], "  ", i + 1 == roots.size());
}
}  // namespace

std::string formatStage1Case(const HrnSectionTestCase& c) {
  if (!c.succeeded || !c.input) return "案例构造失败：" + c.error + "\n";
  std::ostringstream out;
  const Point3D origin = hrn::center(c.input->positionTransform());
  out << "Stage 1 候选输入（尚未布局）\n"
      << "中心：(" << origin.x << ", " << origin.y << ", " << origin.z << ")\n";
  formatSide(out, c, ZeHrnSide::Positive, "正侧");
  formatSide(out, c, ZeHrnSide::Negative, "负侧");
  size_t same_position = 0;
  for (const auto& relation : c.relations)
    if (relation.same_position) ++same_position;
  out << "映射关系（" << c.relations.size() << " 条；同位置 " << same_position << " 条）\n";
  for (const auto& relation : c.relations)
    out << "  正:" << c.graph.labelOf(*relation.positive)
        << " <-> 负:" << c.graph.labelOf(*relation.negative)
        << (relation.same_position ? " [同位置]" : " [普通对应]") << '\n';
  return out.str();
}
}  // namespace hrn
