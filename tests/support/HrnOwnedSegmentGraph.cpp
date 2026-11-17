#include "HrnOwnedSegmentGraph.h"
namespace hrn {
bool HrnOwnedSegmentGraph::tryAdd(const std::string& label, HrnSegment*& result) {
  result = 0;
  if (label.empty() || labels_.count(label)) return false;
  std::unique_ptr<HrnSegment> node(new HrnSegment());
  result = node.get();
  nodes_.push_back(std::move(node));
  labels_[label] = result;
  return true;
}
bool HrnOwnedSegmentGraph::owns(const HrnSegment* p) const {
  for (const auto& n : nodes_)
    if (n.get() == p) return true;
  return false;
}
bool HrnOwnedSegmentGraph::setParent(HrnSegment& child, HrnSegment& parent) {
  return owns(&child) && owns(&parent) && child.attachTo(parent);
}
HrnSegment* HrnOwnedSegmentGraph::find(const std::string& label) const {
  auto it = labels_.find(label);
  return it == labels_.end() ? 0 : it->second;
}
std::string HrnOwnedSegmentGraph::labelOf(const HrnSegment& segment) const {
  for (const auto& entry : labels_)
    if (entry.second == &segment) return entry.first;
  return std::string();
}
size_t HrnOwnedSegmentGraph::depth(const std::string& label) const {
  HrnSegment* p = find(label);
  size_t n = 0;
  while (p) {
    ++n;
    p = p->parent();
  }
  return n;
}
}  // namespace hrn
