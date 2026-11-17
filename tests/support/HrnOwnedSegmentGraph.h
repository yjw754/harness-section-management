#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "hrn/section/HrnSegment.h"
namespace hrn {
class HrnOwnedSegmentGraph {
 public:
  bool tryAdd(const std::string& label, HrnSegment*& result);
  bool setParent(HrnSegment& child, HrnSegment& parent);
  HrnSegment* find(const std::string& label) const;
  size_t depth(const std::string& label) const;
  size_t size() const { return nodes_.size(); }

 private:
  std::vector<std::unique_ptr<HrnSegment>> nodes_;
  std::map<std::string, HrnSegment*> labels_;
  bool owns(const HrnSegment* p) const;
};
}  // namespace hrn
