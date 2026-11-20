#pragma once

#include <map>
#include <vector>

#include "hrn/section/HrnPositionPointInput.h"
#include "hrn/section/HrnSegmentSideRelation.h"

namespace hrn {
// Internal shared-center identities used by both seed packing and coupled checks.
class HrnSamePositionClasses {
 public:
  bool build(const HrnPositionPointInput& input,
             const std::vector<HrnSegmentSideRelation>& relations) {
    positive_count_ = input.sectionCount(ZeHrnSide::Positive);
    const size_t total = positive_count_ + input.sectionCount(ZeHrnSide::Negative);
    leaders_.resize(total);
    edge_count_ = 0;
    std::map<const HrnSegment*, size_t> indices[2];
    for (size_t i = 0; i < total; ++i) {
      leaders_[i] = i;
      const bool positive = i < positive_count_;
      indices[positive ? 0 : 1].emplace(&input.segmentAt(
          positive ? ZeHrnSide::Positive : ZeHrnSide::Negative,
          positive ? i : i - positive_count_), i);
    }
    for (const auto& relation : relations) {
      const auto positive = indices[0].find(relation.positive);
      const auto negative = indices[1].find(relation.negative);
      if (!relation.valid() || positive == indices[0].end() || negative == indices[1].end())
        return false;
      if (!relation.same_position) continue;
      ++edge_count_;
      const size_t a = find(positive->second), b = find(negative->second);
      if (a != b) leaders_[b] = a;
    }
    for (size_t i = 0; i < total; ++i) leaders_[i] = find(i);
    return true;
  }

  size_t group(size_t global_index) const { return leaders_[global_index]; }
  size_t group(ZeHrnSide side, size_t index) const {
    return group((side == ZeHrnSide::Positive ? 0 : positive_count_) + index);
  }
  size_t edgeCount() const { return edge_count_; }

 private:
  size_t find(size_t value) {
    while (leaders_[value] != value) {
      leaders_[value] = leaders_[leaders_[value]];
      value = leaders_[value];
    }
    return value;
  }
  size_t positive_count_ = 0;
  size_t edge_count_ = 0;
  std::vector<size_t> leaders_;
};
}  // namespace hrn
